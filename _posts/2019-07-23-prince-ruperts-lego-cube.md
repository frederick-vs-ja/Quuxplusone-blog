---
layout: post
title: "Prince Rupert's Lego Cube"
date: 2019-07-23 00:01:00 +0000
tags:
  lego
  math
  pretty-pictures
  science
---

Following up on [yesterday's post about Prince Rupert's Cube](/blog/2019/07/22/prince-ruperts-cube/),
I tweaked my quick and dirty volume-calculating program to output Lego bricks in the
[LDRAW format](https://www.ldraw.org/article/218.html).
The new source code is [here](/blog/code/2019-07-23-prince-ruperts-cube-ldraw.cpp), and the resulting LDRAW files
are available for download [here](/blog/code/2019-07-23-prince-rupert-naive.ldr) (the naïve space-diagonal method)
and [here](/blog/code/2019-07-23-prince-rupert-peter.ldr) (Pieter Nieuwland's method).

|:----------------------------------------------------------------:|:--------------------------------------------------------------------:|
| ![Naive method](/blog/images/2019-07-23-prince-rupert-naive.png) | ![Nieuwland method](/blog/images/2019-07-23-prince-rupert-peter.png) |

The Lego visualization program shown in the above screenshots is [LeoCAD](https://www.leocad.org).
I just downloaded it today and have not yet figured out the fine rotation controls, but it looks
like a pretty close fit, right? ...Right?

![Transporter accident](/blog/images/2019-07-23-prince-rupert-passthru.png)

UPDATE: There, this new version looks better. Click to download the LDRAW file.

[![A better version of the passthrough.](/blog/images/2019-07-23-prince-rupert-32x32x80.png)](/blog/code/2019-07-23-prince-rupert-32x32x80.ldr)

----

Note to anyone who tries to build the thing based on these LDRAW files: I made my program use a bunch of differently sized
bricks merely in order to add visual interest and reduce the size of the LDRAW file (compared to using nothing but 1x1 plates).
The thing has no structural integrity at all. In real life, I assume you'd want to use a lot of the biggest beams and plates
you could find.
