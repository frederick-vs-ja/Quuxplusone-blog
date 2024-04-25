---
layout: post
title: "Kotlin AssertJ cheat sheet"
date: 2023-07-28 00:01:00 +0000
tags:
  how-to
  kotlin
---

The [AssertJ documentation](https://assertj.github.io/doc/#assertj-core-common-assertions)
is both hard to find with Google and poorly organized. Here's my attempt to write down
the parts of it that I use (in Kotlin) every couple of months.

This is based on AssertJ version 3.11.1.

    import org.assertj.core.api.Assertions.*

## Scalars

    val b = true
    assertThat(b).isTrue()
    assertThat(!b).isFalse()

    val i = 42
    assertThat(i).isEqualTo(42)
    assertThat(i).isNotEqualTo(24)
    assertThat(i).isLessThan(43)
    assertThat(i).isLessThanOrEqualTo(43)
    assertThat(i).isGreaterThan(41)
    assertThat(i).isGreaterThanOrEqualTo(41)
    assertThat(i).isIn(41, 42, 43)
    assertThat(i).isNotIn(46, 47)

    val n : Int? = null
    assertThat(n).isNull()
    assertThat(i).isNotNull()

## Optionals

    val n : OptionalInt = OptionalInt.of(42)
    assertThat(n).isEmpty()
    assertThat(n).isNotEmpty()
    assertThat(n).hasValue(42)

## Strings

Docs are [here](https://www.javadoc.io/static/org.assertj/assertj-core/3.24.2/org/assertj/core/api/AbstractCharSequenceAssert.html#method-summary).

    val s = "hello world"
    assertThat(s).isEqualTo("hello world")
    assertThat(s).isEqualToIgnoringCase("Hello World")
    assertThat(s).isIn("hello world", "goodbye world")
    assertThat(s).startsWith("he")
    assertThat(s).contains("lo")
    assertThat(s).endsWith("rld")
    assertThat(s).matches("h.*d")
    assertThat(s).doesNotMatch("w.*d")

## Lists

Docs are [here](https://assertj.github.io/doc/#assertj-core-group-assertions).
Assertions on lists can be broken down into assertions about what C++ would call
"sets" (where order and arity are ignored) and assertions about what C++ would
call "sequences" (where order and arity are significant); plus the special case
of `.containsExactlyInAnyOrder` (where arity is significant, but order isn't).

Length:

    val t = listOf(1, 1, 2, 3, 4, 4, 5)
    assertThat(t).isNotEmpty()
    assertThat(t).hasSize(7)
    assertThat(t.size).isGreaterThan(6)

Exact contents:

    assertThat(t).isEqualTo(listOf(1, 1, 2, 3, 4, 4, 5))
    assertThat(t).containsExactly(1, 1, 2, 3, 4, 4, 5)
    assertThat(t).containsExactlyInAnyOrder(3, 1, 4, 1, 5, 2, 4)
    assertThat(t).containsExactlyInAnyOrderElementsOf(listOf(3, 1, 4, 1, 5, 2, 4))

Sets:

    assertThat(t).containsOnly(1, 2, 3, 4, 5)   // set(lhs) == set(rhs)

    assertThat(t).contains(2, 2, 3)             // set(lhs) >= set(rhs)
    assertThat(t).containsAll(listOf(2, 2, 3))  // set(lhs) >= set(rhs)

    assertThat(t).doesNotContain(6, 7)                       // lhs & rhs == set()
    assertThat(t).doesNotContainAnyElementsOf(listOf(6, 7))  // lhs & rhs == set()

Ordered subsequence:

    assertThat(t).containsSequence(2, 3, 4, 4)  // consecutive
    assertThat(t).containsSubsequence(2, 4, 5)  // non-consecutive


## Maps

    val m = mapOf(2 to "abc", 3 to "def", 4 to "ghi")
    assertThat(m).isNotEmpty()
    assertThat(m).containsKey(2)
    assertThat(m).doesNotContainKey(5)
    assertThat(m).containsKeys(2, 3)
    assertThat(m).doesNotContainKeys(5, 6)  // i.e. (m.keys).doesNotContain(5, 6)

You can also deal with `m.keys` directly. These are synonymous:

    assertThat(m).containsOnlyKeys(2, 3, 4)
    assertThat(m.keys).containsExactlyInAnyOrder(2, 3, 4)
    assertThat(m.keys).containsExactlyInAnyOrderElementsOf(listOf(2, 3, 4))

And these are synonymous:

    assertThat(m).containsEntry(2, "abc")
    assertThat(m).contains(entry(2, "abc"))
    assertThat(m[2]).isEqualTo("abc")
