---
layout: post
title: "Slack's new WYSIWYG input box is really terrible"
date: 2019-11-20 00:01:00 +0000
tags:
  rant
  cpplang-slack
  web
---

Slack has just recently rolled out a "WYSIWYG text input" widget to its Web browser interface.
(Apparently, the phased rollout started at the beginning of November 2019, but it's just now starting to
hit the workspaces that I participate in.) The user experience of using this new input method is
really, really, really bad.

First of all, there is no way to go back to plain old Markdown input.
(See @SlackHQ's responses in [this massive Twitter thread](https://twitter.com/SlackHQ/status/1191761054252097541).)
If you prefer the old interface... well, screw you, says Slack.

It wouldn't be a problem if the WYSIWYG interface supported "editing" in the way that Slack users
are used to. But right now a whole lot of stuff is broken — not just "I typed some slightly wrong
sequence of characters and now the text looks messed up," but "I cannot figure out how to recover
the original formatting without deleting my entire message and starting over."

For example: In Markdown, if I have typed

    when you do `foo()` it foos the bar.

it will display, unsurprisingly, as "when you do `foo()` it foos the bar." However, in the new WYSIWYG editor,
it displays as

    when you do `foo() it foos the bar.`

That is, closing backticks are not respected! If you want the proper display, you must hit right-arrow
after the closing backtick (but before the space). That's quite a gymnastic for someone with decades
of muscle memory.

Now suppose you've gotten it displaying right, and now you realize (before hitting Enter) that you really
wanted it to say `bar.foo()` instead of `foo()`.
In the old Markdown interface, I can just left-arrow until the cursor is located immediately before the `f` in `foo`,
and add the new characters `bar.` In the WYSIWYG interface, if you follow that same sequence of steps,
_even though the cursor is clearly displayed inside the code span_ when it's located immediately before the `f`,
what you'll see after typing `bar.` is this:

    when you do bar.`foo()` it foos the bar

I think the only way to insert text at the beginning of a code span in the WYSIWYG editor is
to highlight the first character of the span and type over it (thus cloning all its formatting
onto the new text you're typing).

I wish Slack would provide a way to disable the WYSIWYG rich-text-input box. I don't think it's
useful, and it's extremely annoying to have to keep backspacing to fix mistakes. I'm already starting
to reduce the amount of formatting I use on Slack (e.g., typing "when you do bar.foo() it foos the bar"
without any code highlighting) just so that I can maintain typing speed. But I really don't want to have
to do that! I just want to be able to type Markdown at speed and have it render the way I've grown used to.

If you know someone who works at Slack, please feel free to send them a link to this post!

----

<b>Front-page-of-Hacker-News UPDATE:</b> First of all, whoa! I didn't expect this post
to go [quite this viral](https://www.vice.com/en_us/article/pa7nbn/slacks-new-rich-text-editor-shows-why-markdown-still-scares-people).
But very cool. :)

Funny story: This blog runs on Jekyll, which means I write these posts in Markdown.
I pushed this post so quickly that I didn't notice until a day later that I had accidentally
put both the "raw Markdown" and "rendered" examples above into code blocks, so that readers
were seeing raw Markdown syntax (with backticks) for both the "before" and "after" cases.
Nobody seems to have remarked on that, which I take to mean that the 50,000-some people who
read this blog (before my free plan with Mixpanel stopped tracking the hits) are pretty much
comfortable seeing "<code>bar.`foo()`</code>" and mentally interpreting it, without any loss
of fluency, as "bar.<code>foo()</code>."

When I posted, I had tested my two examples in Safari; I didn't think to check whether they reproduced in other browsers.
As of 2019-11-22, here's what I see in my two browsers of choice:
My first example above reproduces in Safari but not in Chrome; and actually in Safari you have to hit
right-arrow _instead_ of space, not in addition to space.
My second example reproduces in both Safari and Chrome.

Here's a third example, reproducible in both Safari and Chrome. If you type

    increment `self._private_member` by one

into the new WYSIWYG editor, it will display as:

    increment <code>self.<i>private</i>member</code> by one

Here I had to switch from Markdown to HTML in the "rendered" version, because (as far as I know)
there is literally no way to generate "italic teletype text" font in Markdown.
For example, [Alexander Dupuy says:](https://gist.github.com/dupuy/1855764#font-faces---emphasis-and-examples)

> Markdown allows monospaced text within bold or italic sections, but not vice versa

Being a C++ programmer, I use multiple underscores in code _a lot_. I would like them not to be messed with, please.

Finally, as long as I'm getting traffic to this post, this might be the place to mention that
besides [talking about C++ a lot for free](https://www.youtube.com/playlist?list=PLXTVlgmc2KcD3mgkZfrq3jJl8RNaAz-lp),
I also do corporate training! If you're looking for a multi-day training course, with exercises,
on pretty much any aspect of the C++ language, feel free to shoot me an email by clicking on
the leftmost icon below.

----

<b>Partial Victory update:</b> As of 2019-12-03, Slack has added an option to the browser version:
"Preferences > Advanced > Format messages with markup." See full details [here](https://www.howtogeek.com/450030/how-to-enable-classic-markdown-text-formatting-in-slack/)
(Chris Hoffman, 2019-12-03). Setting the option in your "Preferences" for a given workspace will
cause it to carry over to that workspace, in the browser, on any computer. However, setting the option
for one workspace will not affect any other workspace; and setting the option in the browser will
not affect that workspace on the Android mobile app.

On the Android mobile app, "Preferences" is called "Settings", and it's hiding at the bottom of the overflow
menu as described [here](https://slack.com/help/articles/360019434914-Use-dark-mode-in-Slack). It has an
"Advanced" section, but no markdown-related options in there as far as I was able to tell.
