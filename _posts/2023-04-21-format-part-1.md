---
layout: post
title: "`std::format` from scratch, part 1"
date: 2023-04-21 00:01:00 +0000
tags:
  how-to
  library-design
  std-format
excerpt: |
  This is the first post in a three-part series showing how to
  make a simple class type formattable with C++20 `std::format`
  (and, incidentally, the C++98 iostreams way as well).
---

{% raw %}
This is the first post in a three-part series showing how to
make a simple class type formattable with C++20 `std::format`
(and, incidentally, the C++98 iostreams way as well).

These posts assume that you're already vaguely familiar with the
basic way to _use_ `std::format` — e.g. that `std::format("{:.1f}{}x", 3.14, "abc")`
returns a `std::string` with value `"3.1abcx"`.

The series consists of the following posts:

* [Part 1: Basic formatting](/blog/2023/04/21/format-part-1/)
* [Part 2: Handling format specifiers](/blog/2023/04/22/format-part-2/)
* [Part 3: Locale-specific formatting](/blog/2023/04/23/format-part-3/)

----

Here's the type that we'd like to print out. It's just a thin wrapper
around a vector of strings.

    class Widget {
    public:
      explicit Widget(std::vector<std::string> n) :
        names_(std::move(n)) { }
    private:
      std::vector<std::string> names_;
    };

## C++98 iostreams version

To make this type C++98–streamable, we'd do this ([Godbolt](https://godbolt.org/z/nn3PEvv77)):

    class Widget {
    public:
      explicit Widget(std::vector<std::string> n) :
        names_(std::move(n)) { }

      void print(std::ostream& os) const {
        const char *delim = "Widget({";
        for (const auto& name : names_) {
          os << std::exchange(delim, ", ") << std::quoted(name);
        }
        os << "})";
      }

      friend std::ostream& operator<<(std::ostream& os, const Widget& w) {
        w.print(os);
        return os;
      }

    private:
      std::vector<std::string> names_;
    };

We first wrote a member function `Widget::print` that does all the heavy lifting.
Then we wrote a hidden-friend `operator<<` to act as the "glue," the _interface_,
between our implementation in `Widget::print` and callers like this in `main`:

    int main() {
      Widget w({"Håvard", "Howard", "Harold"});
      std::cout << w << '\n';
    }

There's nothing terribly wrong with omitting `Widget::print` and just putting
the implementation straight into `operator<<`. (It's a friend, so it gets access
to `Widget`'s private members already.) But I like to make the whole API consist
of member functions where possible, with a minimum of non-member "glue" as needed.
If you've taken my training courses, you know we do the same thing with
[member `.swap`](https://en.cppreference.com/w/cpp/memory/unique_ptr/swap)
(called from [hidden-friend `swap`](https://en.cppreference.com/w/cpp/memory/unique_ptr/swap2))
and member `.hash` (called from `std::hash<T>`).

## C++20 `std::format` version

To make `Widget` C++20–formattable, we'll do this ([Godbolt](https://godbolt.org/z/oTznoeW5d)):

    class Widget {
    public:
      explicit Widget(std::vector<std::string> n) :
        names_(std::move(n)) { }

      template<class It>
      It format_to(It out) const {
        const char *delim = "Widget({";
        for (const auto& name : names_) {
          out = std::format_to(out, "{}\"{}\"", std::exchange(delim, ", "), name);
        }
        return std::format_to(out, "}})"); // an escaped "})"
      }

    private:
      std::vector<std::string> names_;
    };

Again we've implemented the heavy lifting in a member function, this time named
`format_to`. It takes an arbitrary output iterator,<sup><a href="#note-i-wrote-template-class-it-f">[1]</a></sup>
writes characters into it, and returns the same iterator. We're writing those characters
with `std::format_to`, but we could just as well have pushed characters into the
output iterator manually, like this:

        *out++ = '}';
        *out++ = ')';
        return out;

So we can write a `Widget` directly to `std::cout` like this:

      w.format_to(std::ostream_iterator<char>(std::cout));
      std::cout << '\n';

But we still need some glue code between `Widget::format_to` and the rest of the world.
In C++20, that glue code is a specialization of the `std::formatter` template.
When we `std::format` anything, the library will construct one `std::formatter` object
per format specifier. Each `std::formatter` will be asked to `.parse` its corresponding
format specifier, and then `.format` the matching argument. So we need to implement
both of those methods.

The `.parse` method doesn't really need to do anything, yet, because the only specifier
we'll ever ask it to parse is `"{}"`. It just needs to scan and consume characters until
it hits the format-specifier-terminating `}` character or decides to report a parsing error.
The conventional way to report an error would be to throw `std::format_error` (as we'll
do in Part 2), but here I just used `assert`.

The `.format` method simply needs to call `Widget::format_to` on the output iterator
passed to it. The output iterator is actually bundled up with some other things inside
a [`std::format_context`](https://en.cppreference.com/w/cpp/utility/format/basic_format_context),
and we have to call `ctx.out()` to get at it. What's more, we can't _just_ accept
`std::format_context& ctx`; in order to work with `std::format_to`, we need to accept
`ctx` arguments of all different types. So `std::formatter<Widget>::format` must be a
member function template.

(`.parse` can be templated on the type of its `ctx` too — if you need to handle
`wchar_t` format strings, for example — but that's not typically needed, as far as I know.)

    template<>
    struct std::formatter<Widget> {
      constexpr auto parse(const std::format_parse_context& ctx) {
        auto it = ctx.begin();
        assert(it == ctx.end() || *it == '}');
        return it;
      }
      template<class FormatContext>
      auto format(const Widget& rhs, FormatContext& ctx) const {
        return rhs.format_to(ctx.out());
      }
    };

Finally, we can test `Widget`'s newfound `std::format`-ability with the following `main`:

    int main() {
      Widget w({"Håvard", "Howard", "Harold"});
      std::cout << std::format("{}\n", w);
    }

----

<a name="#fn1">Note:</a> I wrote `template<class It>` for the _template-head_ of
`Widget::format_to`. I certainly could have written `template<std::output_iterator<char> It>`
instead; but that would just be more typing. Also, if I did that, I'd probably feel like
I ought to account for the fact that C++20 output iterators can be _move-only_, which means
I should really have written

    template<std::output_iterator<char> It>
    It format_to(It out) const {
      const char *delim = "Widget({";
      for (const auto& name : names_) {
        out = std::format_to(std::move(out),
            "{}\"{}\"", std::exchange(delim, ", "), name);
      }
      return std::format_to(std::move(out), "}})");
    }

None of that complication buys me anything in ordinary code. The `std::move`s will help
only if I ever try to format a `Widget` into a move-only output iterator; I can't
immediately think of a situation where that would come up, so in ordinary code I wouldn't
bother (although in library code I might).

----

Mark de Wever points out that in C++23, `std::format` will support
[formatting ranges](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2286r8.html)
right out of the box. It'll even print strings double-quoted (and escaped in a
Python-style way) [by default](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2286r8.html#char-and-string-and-other-string-like-types-in-ranges-or-tuples).
So when `__cpp_lib_format_ranges >= 202207L`, we can just write ([Godbolt](https://godbolt.org/z/qsse7443e)):

    template<class It>
    It format_to(It out) const {
      return std::format_to(ctx.out(), "Widget({{{:n}}})", names_);
    }

The doubled `{{` and `}}` represent literal curly braces, in the same way that
a doubled `%%` represents a literal `%` in a `printf` format-specifier.
The `:n` specifier tells the underlying [`std::range_formatter<std::string>`](https://eel.is/c++draft/format.range.formatter)
to omit the square brackets it would usually print around a comma-separated
list of strings: we don't want those square brackets because we're printing
our own curly braces instead.

----

Speaking of non-default format specifiers: Our `.parse` method was pretty trivial
so far; but that's exactly where we'd add code to allow the user-programmer to customize
the formatting of their `Widget`.

In [Part 2](/blog/2023/04/22/format-part-2/), we'll learn how to
customize `Widget`'s formatting logic with a non-trivial format specifier.
{% endraw %}
