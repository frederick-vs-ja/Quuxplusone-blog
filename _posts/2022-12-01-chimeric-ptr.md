---
layout: post
title: "Techniques for post-facto multiple inheritance"
date: 2022-12-01 00:01:00 +0000
tags:
  classical-polymorphism
  library-design
  templates
  type-erasure
---

The other week on the std-proposals mailing list, TPK Healy
[asked](https://lists.isocpp.org/std-proposals/2022/11/5033.php)
for a way of dealing with the following classical-polymorphism scenario,
which arises when dealing with [wxWidgets](https://www.wxwidgets.org/about/).

Throughout this post, I'll use non-wxWidgets-flavored identifiers;
but if you want to translate this example into wxWidgets, the mapping is

* `Text` — `wxTextEntry`
* `Colorful` — `wxControl`
* `set_text` — `SetValue`
* `set_color` — `SetBackgroundColour`
* `Placard`, `Billboard` — `wxComboBox`
* `ColorfulText` — `wxTextCtrl` (sort of)
* `Television` — `wxSearchCtrl`

Suppose we have a bunch of types of widgets in our program. Some of those
widgets involve `Text`, and some of them are `Colorful`. Some of them,
like `Placard` and `Billboard`, are both.

    struct Widget {
        virtual ~Widget() = default;
    };

    struct Text : virtual Widget {
        void set_text() { do_set_text(); }
    private:
        virtual void do_set_text() = 0;
    };

    struct Colorful : virtual Widget {
        void set_color() { do_set_color(); }
    private:
        virtual void do_set_color() = 0;
    };

    struct Placard : Colorful, Text {
    private:
        void do_set_text() override { puts("P::DST"); }
        void do_set_color() override { puts("P::DSC"); }
    };

    struct Billboard : Text, Colorful {
    private:
        void do_set_text() override { puts("B::DST"); }
        void do_set_color() override { puts("B::DSC"); }
    };

Assume that we aren't allowed to change anything above this line.

Now we want to write a _polymorphic free function_ named `set_text_and_color`
that sets the text and color of its argument object. This of course requires
that the argument object be derived from `Text` and also from `Colorful`.
The "obvious" way to do this is to introduce an abstract base class corresponding to
the interface required by our polymorphic function:

    struct ColorfulText : Colorful, Text {};

    void set_text_and_color(ColorfulText *p) {
        p->set_text();
        p->set_color();
    }

    int main() {
        Placard placard;
        set_text_and_color(&placard);
    }

Sadly, this doesn't compile. Although `Placard` does derive from both
`Colorful` and `Text`, it doesn't derive from our made-up intermediate
interface `ColorfulText`.

There are a few ways to make this work, though.


## Solution 1. Templates

The simplest solution is to move `set_text_and_color`'s definition into a header file
and make it a function template. Here I show a C++20 constrained template
(just to prove it's possible), but removing the constraint doesn't break anything.

    template<class T>
        requires std::derived_from<T, Colorful> &&
                 std::derived_from<T, Text>
    void set_text_and_color(T *p) {
        p->set_text();
        p->set_color();
    }

    int main() {
        Placard placard;
        Billboard billboard;
        set_text_and_color(&placard);
        set_text_and_color(&billboard);
    }

This approach simply asks the compiler to stamp out two different functions —
`set_text_and_color(Placard*)` and `set_text_and_color(Billboard*)` —
each with its own entry point and codegen and everything. This requires that
the definition of `set_text_and_color` be visible at the call-site. In practice
that means "templates always go in header files."

Making things into templates isn't always feasible; so let's look at some
non-template alternatives.


## Solution 2. Open-coded `dynamic_cast` via root object type

When you hear "classical polymorphism" and "ad-hoc icky escape hatch,"
you should immediately be thinking "`dynamic_cast`"; and indeed we can
use `dynamic_cast` here, although I don't recommend it. ([Godbolt.](https://godbolt.org/z/3nMhMMz5h))

    void set_text_and_color(Widget *p) {
        dynamic_cast<Text*>(p)->set_text();
        dynamic_cast<Colorful*>(p)->set_color();
    }

    int main() {
        Placard placard;
        Billboard billboard;
        set_text_and_color(&placard);
        set_text_and_color(&billboard);
    }

The function `set_text_and_color(Widget*)` claims to accept any kind of `Widget`,
but in fact it _works_ only for widgets that derive from both `Colorful` and `Text`.


### Non-solution: `dynamic_cast` without root object type

The above implementation works only because we already had a common "root object" type
`Widget` from which both `Placard` and `Billboard` already derived. If such a root
object type didn't exist, as far as I can tell, we'd be out of luck.
You might think (as I did) that we could fix it up with just a light dusting of undefined behavior...

    void set_text_and_color(void *p) {
        struct Dummy { virtual ~Dummy() = default; };
        Dummy *pd = static_cast<Dummy*>(p); // Lie to the compiler
        dynamic_cast<Text*>(pd)->set_text();
        dynamic_cast<Colorful*>(pd)->set_color();
    }

...but in fact this won't work ([Godbolt](https://godbolt.org/z/erEsYao4r)).
The problem is that `dynamic_cast` isn't allowed merely to trace a path up from
the [MDT](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#mdt) to the destination
type (e.g. `Text`) via publicly accessible relationships; it's also required to
trace its path _down_ from the static source type of the pointer (e.g. `Dummy`),
avoiding relationships that are in fact private or protected. That is
([Godbolt](https://godbolt.org/z/q54bKz4qh)):

    struct A { virtual ~A() = default; };
    struct B { virtual ~B() = default; };
    struct C : public A, private B {};

    int main() {
        B *pb = (B*)new C;
        assert(dynamic_cast<A*>(pb) == nullptr);
    }

In practice, when `dynamic_cast` checks to make sure it's not accidentally
pathing through a private relationship, it also naturally rejects any situation
where there is _no_ path from the static source type down to the dynamic MDT.
This situation never arises in conforming C++ code, but it's exactly what we're
doing with `Dummy` above. So this `Dummy` idea doesn't actually work in practice,
let alone in theory!


## Solution 3. Type erasure

The previous approach tried to erase everything about the input type except its
ability to `dynamic_cast` to either `Text*` or `Colorful*`. What if we made that
interface explicit in our code? ([Godbolt](https://godbolt.org/z/fM9TYGrGG).)

    struct ColorfulTextPtr {
        template<class T>
            requires std::derived_from<T, Colorful> &&
                     std::derived_from<T, Text>
        ColorfulTextPtr(T *p) : pc_(p), pt_(p) {}

        Colorful *asColorful() const { return pc_; }
        Text *asText() const { return pt_; }
    private:
        Colorful *pc_;
        Text *pt_;
    };

    void set_text_and_color(ColorfulTextPtr p) {
        p.asText()->set_text();
        p.asColorful()->set_color();
    }

    int main() {
        Placard placard;
        Billboard billboard;
        set_text_and_color(&placard);
        set_text_and_color(&billboard);
    }

Notice that we could generalize `ColorfulTextPtr` into some kind of
ad-hoc-multiple-inheritance-encapsulating pointer template. In the original
std-proposals thread, the name proposed was "`chimeric_ptr<Ts...>`."
[Godbolt](https://godbolt.org/z/x7cGEdxb4):

    template<class... Ts>
    struct ChimericPtr {
        template<class MDT>
            requires (std::derived_from<MDT, Ts> && ...)
        ChimericPtr(MDT *p) : ps_{(Ts*)p...} {}

        template<class T>
            requires (std::same_as<T, Ts> || ...)
        operator T*() const { return std::get<T*>(ps_); }
    private:
        std::tuple<Ts*...> ps_;
    };

    void set_text_and_color(ChimericPtr<Colorful, Text> p) {
        static_cast<Text*>(p)->set_text();
        static_cast<Colorful*>(p)->set_color();
    }

    int main() {
        Placard placard;
        Billboard billboard;
        set_text_and_color(&placard);
        set_text_and_color(&billboard);
    }

Alternatively, we could type-erase even more of the original type,
to the point where we don't even care whether it has that particular
inheritance relationship or not, as long as it supports the `set_text`
and `set_color` member functions. See
["Type-erased `UniquePrintable` and `PrintableRef`"](/blog/2020/11/24/type-erased-printable/) (2020-11-24)
for details of that approach.

One thing this approach loses is the idea that the function argument `p` passed to
`set_text_and_color` ought to _actually be_ a pointer to an object that is both
`Colorful` and `Text`. We had that in the template and `Widget`/`dynamic_cast`-based
approaches, but here our actual argument has changed from a simple pointer to a
class type: `struct ColorfulTextPtr`. Woe betide the naïve client programmer who
tries to `static_cast<void*>(p)` or asks for `typeid(*p)`!


## Solution 4. Adapter Pattern

We can get back that useful inheritance relation of `p` by using an even more
Java-like "design pattern": the
[Adapter](https://english.stackexchange.com/questions/22537/which-is-the-proper-spelling-adapter-or-adaptor) Pattern.
This is essentially the same as the type-erasure approach used by
[`UniquePrintable`](/blog/2020/11/24/type-erased-printable/), but with all the plumbing
exposed to view. We start by defining the polymorphic interface that we wanted
all along:

    struct ColorfulText : Colorful, Text {};

    void set_text_and_color(ColorfulText *p) {
        p->set_text();
        p->set_color();
    }

This polymorphic function will work out of the box with objects that
implement `ColorfulText` directly:

    struct Television : ColorfulText {
    private:
        void do_set_text() override { puts("T::DST"); }
        void do_set_color() override { puts("T::DSC"); }
    };

For all other types, there's `ColorfulTextAdapter` ([Godbolt](https://godbolt.org/z/Ms4Y37fjf)):

    template<class T>
        requires std::derived_from<T, Colorful> &&
                 std::derived_from<T, Text>
    struct ColorfulTextAdapter : ColorfulText {
        explicit ColorfulTextAdapter(T *p) : p_(p) {}
        ColorfulText *addr() { return this; }
    private:
        void set_color() override { p_->do_set_color(); }
        void set_text() override { p_->do_set_text(); }
        T *p_;
    };

    int main() {
        Placard placard;
        Billboard billboard;
        Television television;
        set_text_and_color(ColorfulTextAdapter(&placard).addr());
        set_text_and_color(ColorfulTextAdapter(&billboard).addr());
        set_text_and_color(&television);
    }

----

Previously on this blog:

* ["Remember the `ifstream`"](/blog/2018/11/26/remember-the-ifstream/) (2018-11-26)
