---
layout: post
title: '_Contra_ locales'
date: 2018-04-30 00:02:00 +0000
tags:
  concurrency
  library-design
  mailing-list-quotes
---

Language warning.

Via the SG16 Slack, [this commit message](https://github.com/mpv-player/mpv/commit/1e70e82baa9193f6f027338b0fab0f5078971fbe)
from the `mpv-player` repository (November 2017).

> C locales were utterly moronic even when they were invented. The locale
> (via `setlocale`) is global state, and global state is not a reasonable
> way to do anything. [...]
>
> On top of that, setting a locale randomly changes the semantics of a
> bunch of standard functions. If a function respects locale, you suddenly
> can't rely on it to behave the same on all systems. Some behavior can
> come as a surprise, and of course it will be dependent on the region of
> the user (it doesn't help that most software is US-centric, and the US
> locale is almost like the C locale, i.e. almost what you expect).
>
> Idiotically, locales were not just used to define the current character
> encoding, but the concept was used for a whole lot of things, like e. g.
> whether numbers should use `,` or `.` as decimal separator. The latter
> issue is actually much worse, because it breaks basic string conversion
> [...]

He's right, you know.
