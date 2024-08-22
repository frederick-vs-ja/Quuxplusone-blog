---
layout: post
title: "Andries Brouwer on the OOM killer"
date: 2024-08-22 00:01:00 +0000
tags:
  blog-roundup
  mailing-list-quotes
  sre
---

Via [Andy Miller](https://opsmonkey.blogspot.com/2007/01/linux-memory-overcommit.html) (2007),
an amusing metaphor for Linux memory overcommit. Originally posted by Andries Brouwer to
the linux-kernel mailing list, 2004-09-24, in the thread titled
["oom_pardon, aka don't kill my xlock"](https://lwn.net/Articles/104185/):

> An aircraft company discovered that it was cheaper to fly its planes
> with less fuel on board. The planes would be lighter and use less fuel
> and money was saved. On rare occasions however the amount of fuel was
> insufficient, and the plane would crash. This problem was solved by
> the engineers of the company by the development of a special <b>OOF</b>
> (out-of-fuel) mechanism. In emergency cases a passenger was selected
> and thrown out of the plane. (When necessary, the procedure was
> repeated.)  A large body of theory was developed and many publications
> were devoted to the problem of properly selecting the victim to be
> ejected.  Should the victim be chosen at random? Or should one choose
> the heaviest person? Or the oldest? Should passengers pay in order not
> to be ejected, so that the victim would be the poorest on board? And
> if for example the heaviest person was chosen, should there be a
> special exception in case that was the pilot? Should first class
> passengers be exempted?  Now that the OOF mechanism existed, it would
> be activated every now and then, and eject passengers even when there
> was no fuel shortage. The engineers are still studying precisely how
> this malfunction is caused.

Mel Gorman, in _Understanding the Linux Virtual Memory Manager_ (2004)
[writes](https://www.kernel.org/doc/gorman/html/understand/understand016.html):

> This [the OOM killer] is a controversial part of the VM and it has been suggested
> that it be removed on many occasions. Regardless of whether it exists
> in the latest kernel, it still is a useful system to examine [...]

Twenty years later, as far as I know, the OOM killer is still going strong.
In fact, if you don't like the airline's policy on what counts as an "emergency"
(for example, that it might exhaust your swap partition too before killing any
bad actor at all), you can hire your own hit man, in the form of the userspace daemon
[earlyoom](https://github.com/rfjakob/earlyoom).

For anyone who, like me, didn't instantly understand the ramifications of that
subject line: [`xlock`](https://linux.die.net/man/1/xlock) is a userspace screen-locking
program that blanks your screen until a password is entered, so that you can go get coffee
or whatever without anyone messing with your terminal.
The [original mailing-list thread](https://www.uwsg.indiana.edu/hypermail/linux/kernel/0409.2/1970.html)
started when someone came back to their workstation to find it magically unlocked:
while they were gone, the system had run out of memory and the OOM killer had
chosen to kill the `xlock` process!

For more on the heuristics used to "properly select the victim" these days,
see [the man page for `/proc/[pid]/oom_score_adj`](https://linux.die.net/man/5/proc#:~:text=/proc/[pid]/oom_score_adj%20(since%20Linux%202.6.36)).
