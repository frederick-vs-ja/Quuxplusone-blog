---
layout: post
title: "`std::format` from scratch, part 2"
date: 2023-04-22 00:01:00 +0000
tags:
  how-to
  library-design
  std-format
---

{% raw %}
This is a continuation of yesterday's post, ["`std::format` from scratch, part 1."](/blog/2023/04/21/format-part-1/)
We've already seen how to specialize `std::formatter<Widget>` so that
`std::format("{}", w)` works. Today we're going to add a format specifier, so that
`std::format("{:a}", w)` gives us different behavior.

Recall that a `Widget` looks like this:

    class Widget {
      ~~~~
    private:
      std::vector<std::string> names_;
    };

When we print out a `Widget`, we just print out the contents of its `names_` vector.
But suppose the user frequently wants to print the list in alphabetical order. Let's
allow the user to ask for that with the format specifier `"{:a}"`.

The STL takes care of parsing anything before `:` into a number, so that e.g.
`"{2:a}"` means "print argument number 2 with the `a` format." For example,
`std::format("{1}{0:.1f}x", 3.14, "abc")` returns a `std::string` with value
`"abc3.1x"`.

## With iostreams, just wrap it

With C++98 iostreams, the path of least resistance is probably to create a wrapper type
([Godbolt](https://godbolt.org/z/TG3c9Gv4b)). We saw this technique in Part 1, when we used
`os << std::quoted(name)` to print out `name` with quotation marks around it. `std::quoted`
isn't a state-inducing _manipulator_ of the `ostream`'s own state, like `std::setw` or `std::hex`;
it's just a factory function for a wrapper type with its own special `operator<<`.

First we'll add a `print_sorted` method to `Widget`. (We could instead add a boolean
parameter to our existing `print` method; but see the C++20 section for reasons I didn't
do that.)

    void print_sorted(std::ostream& os) const {
      auto copy = names_;
      std::ranges::sort(copy);
      const char *delim = "Widget({";
      for (const auto& name : copy) {
        os << std::exchange(delim, ", ") << std::quoted(name);
      }
      os << "})";
    }

We create the wrapper type and its factory. I chose to make the factory a static member
of `Widget`, so that the syntax for calling it is `std::cout << Widget::sorted(w)`. Depending
on its purpose, you might prefer to make it a hidden friend, a namespace-level function,
or even a member function: `std::cout << w.sorted()`. The only difference is syntactic.

    struct Sorted {
      const Widget *w_;
      friend std::ostream& operator<<(std::ostream& os, const Sorted& s) {
        s.w_->print_sorted(os);
        return os;
      }
    };

    static Sorted sorted(const Widget& w) {
      return Sorted{&w};
    }

Now we can write a `main` like this:

    Widget w({"Håvard", "Howard", "Harold"});
    std::cout << w << " [unsorted]\n";
    std::cout << Widget::sorted(w) << " [sorted]\n";

Which prints:

    Widget({"Håvard", "Howard", "Harold"}) [unsorted]
    Widget({"Harold", "Howard", "Håvard"}) [sorted]


## With C++20, implement `"{:a}"`

With C++20 formatting, we'll do this ([Godbolt](https://godbolt.org/z/6vr4GGGYb)).
First we'll add a `format_sorted_to` method. (We could instead add a boolean parameter
to our existing `format_to`. But this will set us up better for Part 3, and we
shouldn't [give two different things the same name](https://www.youtube.com/watch?v=OQgFEkgKx2s) anyway.)

      template<class It>
      It format_sorted_to(It out) const {
        auto copy = names_;
        std::ranges::sort(copy);
        const char *delim = "Widget({";
        for (const auto& name : copy) {
          out = std::format_to(out, "{}\"{}\"", std::exchange(delim, ", "), name);
        }
        return std::format_to(out, "}})"); // an escaped "})"
      }

Then we'll extend `std::formatter<Widget>::parse` to handle `'a'`.
When we see an `'a'` in the format specifier, we'll set some member data inside the
`formatter` object that's doing the parsing. That same `formatter` object's
`format` method will be called to format the corresponding `Widget` argument.
Member data is how we pass information from `parse` into `format` — it's why
`parse` is a non-const method.

    template<>
    struct std::formatter<Widget> {
      bool alphabetize_ = false;
      constexpr auto parse(const std::format_parse_context& ctx) {
        auto it = ctx.begin();
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
        if (alphabetize_) {
          return rhs.format_sorted_to(ctx.out());
        } else {
          return rhs.format_to(ctx.out());
        }
      }
    };

Finally, we can write a `main` that tests the new functionality:

    int main() {
      Widget w({"Håvard", "Howard", "Harold"});
      std::cout << std::format("{} with {{}}\n", w);
      std::cout << std::format("{:a} with {{:a}}\n", w);
    }

This prints:

    Widget({"Håvard", "Howard", "Harold"}) with {}
    Widget({"Harold", "Howard", "Håvard"}) with {:a}

The strings are sorted by `std::less<std::string>`; that is, in ordinary
lexicographical order, sometimes called "ASCIIbetical order." The
high-bit `"å"` (UTF-8 `"\xC3\xA5"`) sorts greater than the `"o"` in `"Howard"`.

Tomorrow, we'll learn how to
sort by a locale-specific string collation instead.
{% endraw %}
