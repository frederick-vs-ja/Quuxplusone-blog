---
layout: post
title: "Types that falsely advertise trivial copyability"
date: 2024-05-15 00:01:00 +0000
tags:
  implementation-divergence
  metaprogramming
  pitfalls
  proposal
  triviality
  type-traits
---

I've drafted a paper for the latest WG21 mailing containing the following interesting examples,
and proposing (as yet, vaguely) that we should do something about it.


## A type that falsely advertises trivial copyability

[Godbolt](https://godbolt.org/z/3o5az3vbh):

    struct Plum {
      int *ptr_;
      explicit Plum(int& i) : ptr_(&i) {}
      Plum(const Plum&) = default;
      void operator=(const volatile Plum&) = delete;
      template<class=void> void operator=(const Plum& rhs) {
        *ptr_ = *rhs.ptr_;
      }
    };
    static_assert(std::is_trivially_copyable_v<Plum>);
    static_assert(std::is_assignable_v<Plum&, const Plum&>);
    static_assert(!std::is_trivially_assignable_v<Plum&, const Plum&>);

    int main() {
      int a[3] = {1,2,3};
      int b[3] = {4,5,6};
      Plum pa[3] = {Plum(a[0]), Plum(a[1]), Plum(a[2])};
      Plum pb[3] = {Plum(b[0]), Plum(b[1]), Plum(b[2])};
      std::reverse_copy(pa, pa+3, pb);
      assert(b[0] == 3 && b[1] == 2 && b[2] == 1);
      std::copy(pa, pa+3, pb);
      assert(b[0] == 1 && b[1] == 2 && b[2] == 3);
    }

Microsoft STL's `std::pair<int&, int&>` uses this trick, which I've heard credited to
Peter Dimov and/or Eric Fiselier. We advertise `Plum` as a trivially copyable type,
but in fact we give it semantics that are not value-semantic. This breaks libstdc++'s `copy`
and Microsoft STL's `reverse_copy`.

## A trivially copy-assignable type that breaks the world

[Godbolt](https://godbolt.org/z/1EjKoqErT):

    struct Cat {};
    struct Leopard : Cat {
      int spots_;
      Leopard& operator=(Leopard&) = delete;
      using Cat::operator=;
    };
    static_assert(std::is_trivially_copyable_v<Leopard>);
    static_assert(std::is_trivially_copy_assignable_v<Leopard>);

    void test(Leopard& a, const Leopard& b) { a = b; }

This breaks every STL vendor's `copy` — even libc++, which goes out of its way to double-check
that the type it's copying is not merely `trivially_copyable` but also `trivially_copy_assignable`.
This type does in fact have a trivial copy-assignment operator, which is found by overload resolution.
The trick is, it has _the wrong type's_ trivial copy-assignment operator!


## A non-TC aggregate containing only TC types

[Godbolt](https://godbolt.org/z/7chzjWM1j):

    struct Sunglasses {
      Plum plum_;
    };
    static_assert(!std::is_trivially_copyable_v<Sunglasses>);

You might be surprised that you can detect such "false advertising" simply by wrapping
the type in an aggregate. GCC gets this wrong, but Clang+MSVC+EDG get it right.

This simple trick doesn't quite work on `Leopard`:

    struct PerilSensitiveSunglasses {
      Leopard leopard_;
    };
    static_assert(std::is_trivially_copyable_v<PerilSensitiveSunglasses>);
    static_assert(!std::is_copy_assignable_v<PerilSensitiveSunglasses>);
    static_assert(!std::is_assignable_v<PerilSensitiveSunglasses&, PerilSensitiveSunglasses&>);

MSVC gets this wrong, but Clang+GCC+EDG get it right.


## A TC aggregate containing only non-TC types

Sometimes two wrongs make a right ([Godbolt](https://godbolt.org/z/e8MoPnWdr)):

    struct Fenris {
      Fenris(Fenris&&) = default;
      Fenris(const Fenris&);
      Fenris& operator=(Fenris&&) = default;
      Fenris& operator=(const Fenris&) = delete;
    };
    struct MoonMoon {
      MoonMoon(MoonMoon&&) = default;
      MoonMoon(const MoonMoon&) = delete;
      MoonMoon& operator=(MoonMoon&&) = default;
      MoonMoon& operator=(const MoonMoon&);
    };

    static_assert(!std::is_trivially_copyable_v<Fenris>);
    static_assert(!std::is_trivially_copyable_v<MoonMoon>);

    struct You {
      Fenris f_;
      MoonMoon m_;
    };
    static_assert(std::is_trivially_copyable_v<You>);

Clang+GCC get this wrong, but MSVC+EDG get it right.

---

See also:

* ["When is a trivially copyable object not trivially copyable?"](/blog/2018/07/13/trivially-copyable-corner-cases/) (2018-07-13)
* ["Trivially copyable doesn't mean trivially copy constructible"](https://www.foonathan.net/2021/03/trivially-copyable/) (Jonathan Müller, 2021-03-25)
* ["Trivial, but not trivially default constructible"](/blog/2024/04/02/trivial-but-not-default-constructible/) (2024-04-02)
