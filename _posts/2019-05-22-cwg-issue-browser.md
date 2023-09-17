---
layout: post
title: "A faster WG21 CWG issue browser"
date: 2019-05-22 00:01:00 +0000
tags:
  web
  wg21
---

UPDATE, 2023-09-16: Heroku [dropped support for free plans](https://help.heroku.com/8GD2AVBW/why-am-i-seeing-an-application-error-after-the-termination-of-free-plans)
in November 2022, so the web app described below is dead now. But CWG has followed LWG's lead
and provided their own subpages! As of 2023, you can navigate directly to
[cplusplus.github.io/CWG/issues/1234.html](https://cplusplus.github.io/CWG/issues/1234.html).
So you don't need my dead app anyway.

----

Several weeks ago I made a thing:
[cwg-issue-browser.herokuapp.com/cwg1234](https://cwg-issue-browser.herokuapp.com/cwg1234)
is a faster way to browse WG21 Core Working Group issues by number than the official URL
[www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1234](http://www.open-std.org/jtc1/sc22/wg21/docs/cwg_defects.html#1234).
Because rather than loading and re-rendering 4 megabytes of text every time you load an issue,
this little Heroku app caches those 4 megabytes on the server side and just sends over the
little piece you're interested in.

Subsequent page loads (in
[the 30-minute window before the app goes to sleep again](https://devcenter.heroku.com/articles/free-dyno-hours))
don't even need to re-fetch the 4 megabytes. So if you've already clicked on the first link
above and been unimpressed, maybe you'll be more impressed when you click on
[cwg-issue-browser.herokuapp.com/cwg2345](https://cwg-issue-browser.herokuapp.com/cwg2345).

Props to WG21's LWG for proactively splitting up their issues list into sub-megabyte subpages:
[cplusplus.github.io/LWG/issue1234](https://cplusplus.github.io/LWG/issue1234)
loads super fast.

As usual, the source code is available
[on my GitHub](https://github.com/Quuxplusone/cwg-issue-browser#cwg-issue-browser).

I used the same python+flask+heroku incantations that I'd previously used for
[co.py.cat](http://farg-copycat.herokuapp.com). For writing
little web applications like this, I [used to] highly recommend Heroku!
