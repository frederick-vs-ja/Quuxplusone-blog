---
layout: post
title: "Replacing `std::lock_guard` with a factory function"
date: 2022-12-14 00:01:00 +0000
tags:
  class-template-argument-deduction
  concurrency
  exception-handling
  implementation-divergence
  library-design
  llvm
  templates
  war-stories
excerpt: |
  Recently I had to track down a use-after-free bug ([which see](https://github.com/confluentinc/librdkafka/issues/4100)) in some code that used librdkafka.
  The gist of it was: We call `rd_kafka_new` and pass it an `rk_conf` object
  containing a bunch of pointers to callbacks. `rd_kafka_new` either succeeds, in which
  case it gives us back a valid `rd_kafka_t*` and we make sure to keep those callback
  objects alive; or it fails (e.g. due to invalid configuration or resource exhaustion),
  in which case it gives us back `NULL` and we throw a C++ exception (and destroy those
  callback objects). But — here's the bug, as far as I was able to tell — _sometimes_
  `rd_kafka_new` spawns a broker thread and then returns `NULL` anyway, without cleaning
  up the broker thread. And then later on, the broker thread attempts to call one of those
  callbacks... which we've already destroyed, because `rd_kafka_new` had told us it failed!
  So that was our use-after-free issue.

  This use-after-free bug announced itself via an unusual symptom: an uncaught `std::system_error`
  exception.
---

Recently I had to track down a use-after-free bug ([which see](https://github.com/confluentinc/librdkafka/issues/4100)) in some code that used librdkafka.
The gist of it was: We call `rd_kafka_new` and pass it an `rk_conf` object
containing a bunch of pointers to callbacks. `rd_kafka_new` either succeeds, in which
case it gives us back a valid `rd_kafka_t*` and we make sure to keep those callback
objects alive; or it fails (e.g. due to invalid configuration or resource exhaustion),
in which case it gives us back `NULL` and we throw a C++ exception (and destroy those
callback objects). But — here's the bug, as far as I was able to tell — _sometimes_
`rd_kafka_new` spawns a broker thread and then returns `NULL` anyway, without cleaning
up the broker thread. And then later on, the broker thread attempts to call one of those
callbacks... which we've already destroyed, because `rd_kafka_new` had told us it failed!
So that was our use-after-free issue.

This use-after-free bug announced itself via an unusual symptom: an uncaught `std::system_error` exception.

Here's the code, more or less.
This `UniquePtr` ownership pattern was previously presented in
["OpenSSL client and server from scratch"](/blog/2020/01/24/openssl-part-1/) (2020-01-24)
and ["Back to Basics: Smart Pointers"](https://www.youtube.com/watch?v=xGDLkt-jBJ4&t=11m33s) (CppCon 2019).

    template<class T> struct DeleterOf;
    template<> struct DeleterOf<rd_kafka_t> { void operator()(rd_kafka_t *p) const { rd_kafka_destroy(p); } };
    template<> struct DeleterOf<rd_kafka_conf_t> { void operator()(rd_kafka_conf_t *p) const { rd_kafka_conf_destroy(p); } };
    template<class T> using UniquePtr = std::unique_ptr<T, my::DeleterOf<T>>;

    struct Producer {
        explicit Producer();
    private:
        void log(const char *msg) {
            auto lk = std::lock_guard(mutex_);
            ~~~~
        }

        my::UniquePtr<rd_kafka_t> rk_;
        std::mutex mutex_;
    };

    Producer::Producer() {
        auto rk_conf = my::UniquePtr<rd_kafka_conf_t>(rd_kafka_conf_new());
        ~~~~
        rd_kafka_conf_set_opaque(rk_conf.get(), this);
        rd_kafka_conf_set_log_cb(
            rk_conf.get(),
            [](const rd_kafka_t *rk, int level, const char *, const char *msg) {
                Producer *self = static_cast<Producer *>(rd_kafka_opaque(rk));
                self->log(msg);
            }
        );
        char errbuf[512];
        rk_ = my::UniquePtr<rd_kafka_t>(rd_kafka_new(RD_KAFKA_PRODUCER, rk_conf.get(), errbuf, sizeof errbuf));
        if (rk_ == nullptr) {
            throw my::ProducerError(errbuf);
        } else {
            rk_conf.release(); // rk_ owns it now
        }
    }

`Producer::log` doesn't blindly log the message; the "`~~~~`" above hides some code that will
update various statistical counts, and suppress messages that have been seen too many times
in the last few seconds, and so on. In other words, `Producer::log` mutates the object's state;
and it might be called concurrently from multiple broker threads inside librdkafka; therefore
it takes a lock on `mutex_` to avoid data races.


## How a use-after-free becomes a `system_error`

As I said above, _sometimes_ it happens that `rd_kafka_new` returns `NULL` but leaks a thread
that will later call the logging callback anyway. In that case, `Producer::Producer()` will have
seen the `NULL` and thrown `my::ProducerError`, and so when the logging callback is finally
called, `Producer *self` points to a `Producer` object that is no longer alive (in fact,
technically, it never was, since its constructor did not succeed).

We heap-allocated this `Producer` object, so "no longer alive" means "deallocated."
Our memory allocator helpfully scribbles `0xdededede` over the deallocated block.
So, when the logging callback is finally called, and it calls `self->log(msg)`,
and `log` constructs its `lock_guard`, we're calling `lock()` on a `std::mutex` with the
nonsense bit-pattern of `0xdededede`.

Now we descend into the Standard Library's implementation of `mutex::lock()`. This is OS X,
so (as usual on this blog) I'm talking about libc++. libc++'s `mutex::lock()` looks essentially like
[this](https://github.com/llvm/llvm-project/blob/f87aa19be64499308fc18f92d048c5fa2d3064d9/libcxx/src/mutex.cpp#L35-L41):

    void mutex::lock() {
        if (int ec = pthread_mutex_lock(&__m_)) {
            auto temp = std::error_code(ec, std::system_category());
            throw std::system_error(temp, "mutex lock failed");
        }
    }

And of course if you try to `pthread_mutex_lock` a mutex with the nonsense bit-pattern
of `0xdededede`, you can expect that it'll return `EINVAL` and so libc++ will throw a
`std::system_error` with the what-string `"mutex lock failed: invalid argument"`.
Which will propagate up to the top of whatever librdkafka-internal broker thread
triggered the use-after-free, and then `std::terminate`.


## "Failing faster" with a factory function

A use-after-free is definitely a logic bug in the program, not a runtime condition that
can be "handled" by a catch handler. So rather than throw an exception, we'd really prefer
to assert-fail as soon as possible, get a coredump, and go fix the bug. So we want to
wrap all our uses of `mutex::lock()` in a `try`/`catch`/`assert`.

Fortunately, _most_ of our uses of `lock()` are already wrapped — in `std::lock_guard`
or `std::unique_lock`! So where our codebase used to do

    auto lk = std::lock_guard(m);

it now does this, for faster failures:

    auto lk = my::lock_guard(m);

Whereas `std::lock_guard` is a class template that uses CTAD, `my::lock_guard` is a
plain old function template, like `std::ref` or `std::back_inserter`. It looks like
this:

    namespace my {

    template<class V = void, class Mutex, class... Args>
    std::lock_guard<Mutex> lock_guard(Mutex& m, Args&&... args) {
        static_assert(std::is_void_v<V>, "No explicit template arguments, please!");
        try {
            return std::lock_guard<Mutex>(m, std::forward<Args>(args)...);
        } catch (const std::system_error&) {
            MY_ASSERT(false);
        }
    }

    } // namespace my

The implementation of `my::unique_lock` is cut-and-paste identical.

We forward our `args...` to the constructor of `lock_guard`, in order to support
places in the codebase that do things like

    auto lk = my::lock_guard(m, std::adopt_lock);

or

    std::timed_mutex tm;
    auto lk = my::lock_guard(tm, std::chrono::seconds(2));

The trick with template parameter `V` is probably overkill, but I just wanted
to enforce that people weren't calling `lock_guard` with explicit template arguments:

    auto lk = my::lock_guard<std::mutex>(m);  // Error, please!

C++ templates generally have one set of template parameters that _must_ be
explicitly passed (like the `T` in `make_unique<T, Args...>`) and
one set that _should_ be deduced (like the `Args` in `make_unique<T, Args...>`,
or the `T, U` in `make_pair<T, U>`).
As of C++23, there's no dedicated syntax to distinguish one kind of parameter
from the other, so a template author who really wants to _enforce_ a distinction
(to guide callers away from misuse) must resort to this kind of hacky trick.

Part of the benefit of `my::lock_guard` is that there's basically only one
natural way to use it (shown above); whereas with `std::lock_guard` you have
at least four plausible spellings:

    std::lock_guard<std::mutex> lk(m); // C++11-friendly
    std::lock_guard lk(m);
    auto lk = std::lock_guard<std::mutex>(m);
    auto lk = std::lock_guard(m);  // my preference

(and of course sillier things like `std::lock_guard lk{m}`,
but let's not enumerate those). With `my::lock_guard`, only one
of those spellings will compile. That's a good thing!

Another way to fat-finger a standard lock type is:

    auto lk = std::unique_lock<std::mutex>();

When you're forced to rewrite that as

    auto lk = my::unique_lock();

the missing `m` argument not only stands out, but actually produces a compiler error!

As a completely incidental side benefit, switching to these factory functions
removes the last (intentional) use of CTAD from this particular codebase,
meaning that we can finally [switch on `-Wctad`](https://p1144.godbolt.org/z/MnnhMTz8G)
(in our experimental fork of Clang) and find all the _unintentional_ uses of CTAD!

> For the record, I found only two other uses of CTAD in our codebase. Neither was
> clearly "unintentional," but both were correlated with poor code quality.
> (1) Unit-test inputs using `std::array x =` instead of `const char *x[] =`,
> incidentally associated with an off-by-one bug in the general vein of
> ["The array size constant antipattern"](/blog/2020/08/06/array-size/) (2020-08-06).
> (2) Unit-test code using `std::set(x.begin(), x.end())` to convert a `list` to
> a `set`; I refactored it to create a `set` instead of a `list` in the first place.


## Caveats, of course

I said that _most_ of our uses of `lock()` are already wrapped in `std::lock_guard`
or `std::unique_lock`. But some aren't. For example, `cv.wait(lk)` will call
`.unlock` and `.lock` on the underlying mutex; we can't interpose on that; and
so a use-after-free that happens inside a `wait` will still cause a throw (on libc++)
instead of our assert-fail.

In fact, it's a little worse than that: libc++'s `std::condition_variable`
goes straight to `pthread_cond_wait` and thus doesn't turn "invalid argument"
into a `system_error` as far as I can tell. And libc++'s `std::condition_variable_any`
uses enough RAII that as its `system_error` propagates up through the library code,
it ends up doing _another_ pthreads operation on the same mutex, which throws a
_second_ `system_error`. Having two exceptions in-flight at the same time isn't
allowed, so we jump straight to `std::terminate`.

As a concrete example, on my laptop, the following program's UB manifests as an
un-`catch`-able call to `std::terminate`. (On Godbolt, it hangs instead.)

    int main()
        std::variant<std::mutex, int> v;
        std::mutex& m = std::get<0>(v);
        std::condition_variable_any cv;
        bool ready = false;
        auto t = std::thread([&]() {
            auto lk = my::unique_lock(m);
            while (!ready) {
                cv.wait(lk);
            }
        });
        std::this_thread::yield();
        {
            auto lk = my::lock_guard(m);
            ready = true;
        }
        v.emplace<1>(0xdededede); // invalidate `m`
        cv.notify_all();  // `t` will try to re-lock `m`
        t.join();
    }

And of course all of this use-after-free stuff _is_ undefined behavior;
`my::lock_guard` is simply trying to slap some flimsy paper over the
barn door long after the horses have escaped.

This `system_error` saga was the last straw that pushed me to finally
implement `my::lock_guard` for real, move the codebase over to it, and
turn on `-Wctad` in my experimental Clang branch. `std::lock_guard` and
`std::unique_lock` had been the last best hope for CTAD's place in a
modern production codebase... but now I've got a practically relevant reason
never to directly construct those types at all. So, goodbye from our
codebase, CTAD!
