---
layout: post
title: "Mathematical golf"
date: 2023-03-23 00:01:00 +0000
tags:
  board-games
  math
  old-shit
  puzzles
excerpt: |
  Item 43 in [Clement Wood](/blog/2023/01/02/pangrams/)'s 74-item _Book of Mathematical Oddities_ (1927)
  is titled "A Matter of Golf":

  > The nine holes of a mathematical golf course are
  > 300, 250, 200, 325, 275, 350, 225, 375, and 400 yards apart.
  > If a man could always strike the ball in a perfectly straight
  > line exactly one of two distances, so that it could either go
  > toward the hole, or pass over it, or drop into it, what would
  > the two distances be that would carry him in the least number
  > of strokes around the whole course? How many strokes would it
  > require?

  Solution below the break.
---

Item 43 in [Clement Wood](/blog/2023/01/02/pangrams/)'s 74-item _Book of Mathematical Oddities_ (1927)
is titled "A Matter of Golf":

> The nine holes of a mathematical golf course are
> 300, 250, 200, 325, 275, 350, 225, 375, and 400 yards apart.
> If a man could always strike the ball in a perfectly straight
> line exactly one of two distances, so that it could either go
> toward the hole, or pass over it, or drop into it, what would
> the two distances be that would carry him in the least number
> of strokes around the whole course? How many strokes would it
> require?

<b>Solution:</b> Choose a 125-yard driver and a 100-yard putter.
Then you can cover the nine holes in 3, 2, 2, 3, 4, 3, 2, 3,
and 4 strokes respectively, for a total of 26 strokes. (On the
fifth hole, $$275 = 3\times 125 - 100$$.)

The general case here could be a pretty interesting mathematical recreation
for long car trips. Let one person propose a "course" of several smallish
numbers; for example 10, 14, 17, 28. Each player then quietly chooses
their own "driver" and "putter" numbers. Then players tally their stroke
count for each hole, and the low score wins.

For "10, 14, 17, 28" I think everyone would choose 10 and 7, covering the
four holes in 1, 2, 2, and 4 strokes respectively. So that was an easy course.
Now how about "10, 14, 18, 28"?

For "10, 14, 18, 28" I'd choose 10 and 8, covering the four holes in
1, 4, 2, and 3 strokes respectively, for a score of 10. But the best choice
for this course is actually 14 and 4, which scores 2, 1, 2, 2 respectively for
a score of 7 strokes!

Finally, consider the course "10, 15, 16, 29."
I'd stare perplexedly out over the course for several minutes before shrugging
and choosing a 5 driver and a 4 putter (for a total of 16 strokes). But in fact
you can do much better than I did! Can you finish this course in only 11 strokes?

----

If the basic game is too simple, you might consider adding the rule that
the ball will still drop into the hole even if you overshoot by 1; but of course
not if you undershoot by 1.

Now the best score for "10, 14, 17, 28" drops from 9 to 7.
And "10, 15, 16, 29" can now be done in 8 strokes! Do you see how?
