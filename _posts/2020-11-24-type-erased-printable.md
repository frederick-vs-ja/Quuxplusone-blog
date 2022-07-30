---
layout: post
title: "Type-erased `UniquePrintable` and `PrintableRef`"
date: 2020-11-24 00:01:00 +0000
tags:
  pearls
  slack
  type-erasure
---

Here's a simple type-erased `UniquePrintable` (owning, value-semantic, move-only),
and a simple type-erased `PrintableRef` (non-owning, reference-semantic, trivially copyable).
Notice that they use two different techniques internally, and that _both_ techniques are
simple enough to memorize and bang out in five minutes the next time you need type erasure.

You could use the virtual-methods technique to implement `PrintableRef`, but you'd lose
trivial copyability. You could use the function-pointer technique to implement `UniquePrintable`,
but at the cost of more complicated move-constructor logic. Notice that `UniquePrintable`
is able to follow the Rule of Zero because it delegates all the ownership logic to `std::unique_ptr`.
A copyable `Printable` type would have a user-defined copy constructor, which makes it just
complicated enough that I won't show it here. But see my talk
["Back to Basics: Type Erasure"](https://www.youtube.com/watch?v=tbUCHifyT24) (CppCon 2019)
for several implementations of a copyable type-erased `std::any`.


## `UniquePrintable`

[Godbolt.](https://godbolt.org/z/rb8WTe)

    struct PrintableBase {
        virtual void print(std::ostream& os) const = 0;
        virtual ~PrintableBase() = default;
    };

    template<class T>
    struct PrintableImpl : PrintableBase {
        T t_;
        explicit PrintableImpl(T t) : t_(std::move(t)) {}
        void print(std::ostream& os) const override { os << t_; }
    };

    class UniquePrintable {
        std::unique_ptr<PrintableBase> p_;
    public:
        template<class T>
        UniquePrintable(T t) : p_(std::make_unique<PrintableImpl<T>>(std::move(t))) { }

        friend std::ostream& operator<<(std::ostream& os, const UniquePrintable& self) {
            self.p_->print(os);
            return os;
        }
    };


### Example of use

    void printit(UniquePrintable p) {
        std::cout << "The printable thing was: " << p << "." << std::endl;
    }

    int main() {
        printit(42);
        printit("hello world");
    }

-----

## `PrintableRef`

[Godbolt.](https://godbolt.org/z/GTsK5c)

    class PrintableRef {
        const void *data_;
        void (*print_)(std::ostream&, const void *);
    public:
        template<class T>
        PrintableRef(const T& t) : data_(&t), print_([](std::ostream& os, const void *data) {
            os << *(const T*)data;
        }) { }

        friend std::ostream& operator<<(std::ostream& os, const PrintableRef& self) {
            self.print_(os, self.data_);
            return os;
        }
    };


### Example of use

    void printit(PrintableRef p) {
        std::cout << "The printable thing was: " << p << "." << std::endl;
    }

    int main() {
        printit(42);
        printit("hello world");
    }

----

See also:

* ["What is Type Erasure?"](/blog/2019/03/18/what-is-type-erasure/) (2019-03-18)
* ["Back to Basics: Type Erasure"](https://www.youtube.com/watch?v=tbUCHifyT24) (CppCon 2019)
* ["The space of design choices for `std::function`"](/blog/2019/03/27/design-space-for-std-function/) (2019-03-27)
* ["Type-erased `InplaceUniquePrintable` benefits from `noexcept`"](/blog/2022/07/30/type-erased-inplace-printable/) (2022-07-30)
