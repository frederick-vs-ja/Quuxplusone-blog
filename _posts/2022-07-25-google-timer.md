---
layout: post
title: "Google Timer is dead; long live BigTimer"
date: 2022-07-25 00:01:00 +0000
tags:
  conferences
  rant
  web
---

Yesterday I made a revolting discovery: Google Timer has been quietly discontinued!
For years and years, you used to be able to google a phrase like "5 minute timer"
and receive — in a privileged position above the Google search results — a little
JavaScript widget with a countdown. You could take this tab, pull it out into another
little browser window, and stick it in the corner of your screen, to produce an
on-screen timer counting down whatever you needed. In the context of a training
course, it might be counting down the 30 minutes for a lab exercise, the 8 minutes
for a coffee break, the hour for lunch... Having an easily accessible on-screen
timer widget, visible to all the attendees, was _fantastic_.

> Having an easily accessible way to do simple math is also fantastic, by the way.
> When I need to add up 1+3+50+43+27+4, I can just [google it](https://www.google.com/search?q=1%2B3%2B50%2B43%2B27%2B4).
> This is usually easier than spinning up `python` in a terminal or whatever.

Well, apparently, as of July 18, [Google Timer is gone!](https://9to5google.com/2022/07/22/google-search-timer-stopwatch/)
Now, when you google "5 minute timer," you get a widget-free search results page:
just a ton of bot-generated YouTube videos, "People also ask..." suggestions
(prominently including _Where is the Google Timer?_ and _Did Google remove Timer?_),
and JavaScript timer apps with banner ads all over them.

[According to 9to5google](https://9to5google.com/2022/07/22/google-search-timer-stopwatch/),
the removal might have been an accident (what?), so maybe it's coming back eventually.
But until then, here are two potential replacements:

* [CppConTimer](https://quuxplusone.github.io/CppConTimer/index.html?duration=30m&show_seconds=true),
    originally written by Jon Kalb and myself. This is really intended for non-interactive use
    by the volunteer at the back of the room during a conference talk (CppCon and C++Now both
    use it), so it doesn't spend any screen real estate on settings; everything is done by URL-hacking.
    And of course it doesn't interrupt the speaker with an alarm noise, although it does change
    the text color and flash the screen several times as you approach zero.
    See [the documentation](https://github.com/Quuxplusone/CppConTimer#cppcontimer).

* [BigTimer.net](https://www.bigtimer.net/?minutes=30&repeat=false), no obvious owner (but also
    no obvious banner ads, yet). This UI is pretty close to Google Timer's — you can click on the
    digits to pause or reset the timer, and it makes a nice satisfying bwonggg noise at zero.
    This is what I'm going to be using in my classes from now on... unless I find something
    better.
