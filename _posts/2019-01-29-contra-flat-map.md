---
layout: post
title: "_Contra_ P0429R6 `flat_map`"
date: 2019-01-29 00:01:00 +0000
tags:
  flat-containers
  kona-2019
  library-design
  sg14
  wg21
---

Yesterday I wrote:

> [P0429R6 "A Standard `flat_map`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0429r6.pdf):
> I don't honestly think `flat_map` is well-baked enough to put into _the_ Standard Library; but at least
> it's been in Boost for a while (if I understand correctly)...

The author of Boost.FlatMap, Ion Gaztañaga, informs me via email that my understanding of
`flat_map`'s Boostness is incorrect.

> Between [P0429R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0429r1.pdf)
> and [P0429R2](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0429r2.pdf)
> there is an important change: the layout of flat_map changes to store
> keys and mapped types separately [...]
>
> [...] I'd say there is no implementation experience in Boost for the
> proposed `flat_map` at all. It is possible, but I haven't checked, that
> other libraries use the [split-storage] approach. However, apart
> from Boost.FlatMap, Alexandrescu's original [`AssocVector`](http://loki-lib.sourceforge.net/html/a00645.html),
> ETL's [`flat_map`](https://www.etlcpp.com/flat_map.html), and Folly's
> [`sorted_vector_map`](https://github.com/facebook/folly/blob/master/folly/sorted_vector_types.h)
> are based on `std::pair<Key,T> value_type`s and real references.
> I think there is widespread existing practice on this design.
>
> I honestly think, unless there are some important performance reasons,
> that separate keys and values are problematic, at least if we want to
> have an interface similar to `std::map`. It breaks references to
> `value_type`s, requires proxy types, and complicates the adapter.

(Ion points to the benchmarks in [P0429R1](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0429r1.pdf)
as evidence that those "important performance reasons" have not materialized.)

Ion also frowns on P0429R1's removal of the `reserve`, `capacity`, and `shrink_to_fit` methods.
The reason to remove them is that some custom containers don't have these methods, and we don't
want to prevent the programmer from using such containers as the underlying container of a
`flat_map`. It's important to keep the surface area of the underlying container as small as possible.

Of the four prior-art implementations listed above, only Folly permits using a custom container at all.
Folly provides `capacity`, `reserve`, and `shrink_to_fit` unconditionally, which means your custom
container type _must_ provide those methods.

_But_, we can always provide those members _conditionally_, so that the
`flat_map` can be `reserve`able if-and-only-if the underlying container is `reserve`able! SG14's
[`slot_map`](https://github.com/WG21-SG14/SG14/blob/2a756bff57/SG14/slot_map.h#L206-L210)
uses this exact technique for `reserve` and for `capacity` (and will for `shrink_to_fit` whenever
I get around to making the pull request). So it's quite surprising that P0429 `flat_map` doesn't
provide the same methods conditionally.
[We have the technology!](https://www.youtube.com/watch?v=HoLs0V8T5AA&t=35s)

So, I retract my "at least it's been in Boost for a while" comment. <b>There is no prior implementation
experience for P0429 `flat_map`</b>... and as far as I can tell, there is no reference implementation, either.
(Proposal author Zach Laine [has a `flat_map` repository on GitHub](https://github.com/tzlaine/flat_map/),
but it does not contain any implementation — just the TeX source of the paper proposal.)

If WG21 standardizes P0429 `flat_map` without any implementation, and _against_ all existing implementations,
it'll really be striking out on its own — in both senses. I advise strongly against that course of action;
I believe P0429 should be tabled until an implementation can be created and some implementation experience
gained.

And I volunteer to work on an implementation in the SG14 repo!
