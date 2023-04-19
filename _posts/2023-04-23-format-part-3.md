---
layout: post
title: "`std::format` from scratch, part 3"
date: 2023-04-23 00:01:00 +0000
tags:
  how-to
  library-design
  std-format
---

{% raw %}
This is a continuation of yesterday's post, ["`std::format` from scratch, part 2."](/blog/2023/04/22/format-part-2/)
We've already seen how to specialize `std::formatter<Widget>` so that we can
`std::format("{}", w)`, and how to implement format specifiers so that we can
`std::format("{:a}", w)`. Today we're going to implement an `"{:La}"` format
specifier that does locale-sensitive sorting.

First of all: [Locales are terrible.](/blog/2018/04/30/contra-locales/)
Nothing about this has changed in C++20. You shouldn't make _any_ of your program's
behavior locale-dependent, if you can possibly help it.

Still, for built-in types (such as floats), `std::format` generally permits you to
opt into locale-dependent formatting behavior (such as the European use of `,` for decimal point)
via the `L` specifier. There are two sides to this support:

* The caller _may_ pass a `std::locale` object as the first argument of `std::format`.
    If they do, it will be retrievable from the `format_context` as `ctx.locale()`.
    Otherwise, `ctx.locale()` will return `std::locale()` — a copy of
    [the global locale](https://en.cppreference.com/w/cpp/locale/locale/global).

* Each format specifier may _opt in_ to locale-dependent formatting by including the `L`
    specifier. (This is just a convention, but it's one you should follow, too.)
    Specifiers without `L` do locale-independent formatting. Specifiers with `L` do
    locale-dependent formatting according to `ctx.locale()`.

In other words, `std::format("foo", args...)` always behaves the same as
`std::format(std::locale(), "foo", args...)`. And `std::format(loc, "foo", args...)`
ignores the locale _except_ for format-specifiers that involve `L`.

* `std::format("{:.2f}", 3.14)` is invariably `"3.14"`
* `std::format(std::locale("en_US"), "{:.2Lf}", 3.14)` is invariably `"3.14"`
* `std::format(std::locale("da_DK"), "{:.2Lf}", 3.14)` is invariably `"3,14"`
* `std::format("{:.2Lf}", 3.14)` is `"3.14"` or `"3,14"` depending on the current locale

Let's implement a formatter for `Widget` with the following behavior:

* `std::format("{:a}", w)` sorts according to the `"C"` locale (as we did yesterday)
* `std::format(std::locale("en_US"), "{:La}", w)` sorts according to the `"en_US"` locale
* `std::format(std::locale("da_DK"), "{:La}", w)` sorts according to the `"da_DK"` locale
* `std::format("{:La}", w)` sorts according to the current locale


## Implement it!

We change our `format_sorted_to` method to take a comparator ([Godbolt](https://godbolt.org/z/drbaKoE5T)):

      template<class It, class Comp>
      It format_sorted_to(It out, Comp less) const {
        const char *delim = "Widget({";
        auto copy = names_;
        std::ranges::sort(copy, less);
        for (const auto& name : copy) {
          out = std::format_to(out, "{}\"{}\"", std::exchange(delim, ", "), name);
        }
        return std::format_to(out, "}})"); // an escaped "})"
      }

Then we change our `std::formatter` specialization to pass either
`std::less<>()` (for ordinary ASCIIbetical sort order) or `ctx.locale()`
(the `locale` argument, if any, passed by the caller of `std::format`).
Conveniently, [`std::locale` is usable as a comparator](https://en.cppreference.com/w/cpp/locale/locale/operator()).

    template<>
    struct std::formatter<Widget> {
      bool alphabetize_ = false;
      bool use_locale_ = false;
      constexpr auto parse(const std::format_parse_context& ctx) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == 'L') {
          use_locale_ = true;
          ++it;
        }
        if (it != ctx.end() && *it == 'a') {
          alphabetize_ = true;
          ++it;
        }
        if (it != ctx.end() && *it != '}') {
          throw std::format_error("invalid format for Widget");
        }
        return it;
      }
      template<class FormatContext>
      auto format(const Widget& rhs, FormatContext& ctx) const {
        if (alphabetize_ && use_locale_) {
          return rhs.format_sorted_to(ctx.out(), ctx.locale());
        } else if (alphabetize_) {
          return rhs.format_sorted_to(ctx.out(), std::less<>());
        } else {
          return rhs.format_to(ctx.out());
        }
      }
    };

Finally, we add a `main` for testing:

    int main() {
      std::locale::global(std::locale("")); // use the environment's locale

      Widget w({"Håvard", "Howard", "Harold"});
      std::cout << std::format("{} with {{}}\n", w);
      std::cout << std::format("{:a} with {{:a}}\n", w);
      std::cout << std::format("{:La} with {{:La}} in current locale\n", w);
      std::cout << std::format(std::locale("da_DK"), "{:La} with {{:La}} in Danish locale\n", w);
    }

When invoked with environment variables selecting an unusual locale, this prints:

    $ LC_ALL=en_US.ISO8859-1 ./a.out
    Widget({"Håvard", "Howard", "Harold"}) with {}
    Widget({"Harold", "Howard", "Håvard"}) with {:a}
    Widget({"Håvard", "Harold", "Howard"}) with {:La} in current locale
    Widget({"Harold", "Håvard", "Howard"}) with {:La} in Danish locale

----

That concludes my three-day, three-part blog series on `std::format`.
I hope you enjoyed it!

To start again at the beginning, go back to [part 1.](/blog/2023-04-21-std-format-part-1/)
{% endraw %}
