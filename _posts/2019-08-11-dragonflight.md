---
layout: post
title: "Dragonflight, and running DosBox in the browser"
date: 2019-08-11 00:01:00 +0000
tags:
  digital-antiquaria
  playable-games
  web
---

Another of the things I pulled off of my old floppy disks
(["Making floppy disk images under OS X"](/blog/2019/07/26/disk-images-in-os-x/), 2019-07-26)
was a game called "Dragonflight" which I wrote in Turbo Pascal, dateline September 1999.
These days we'd call it a _Flappy Bird_ clone.
There's no win condition; you just flap along until your health drops to zero or you get bored.
Hit F1 (or alternatively, `';'`) to bring up the help screen.

<canvas id="jsdos" width="640px" height="400px"></canvas>
<script src="/blog/code/js-dos.js"></script>
<script>
  Dos(document.getElementById("jsdos"), {
    wdosboxUrl: "/blog/code/wdosbox.js",
  }).ready((fs, main) => {
    fs.extract("/blog/code/2019-08-11-dflight.zip").then(() => {
      main(["-c", "loop.bat"])
    });
  });
</script>

The ASCII art is credited to "Gunnar Z." and "Unknown." You can still find Gunnar Z.'s
little dragon sprite in various places online, such as [here](http://ascii.co.uk/art/dragons).
I believe the "Unknown" refers to the dragon on the title screen, which I vaguely recall might
have been lifted from some anonymous _Dragonriders of Pern_ fan art.


## Running DosBox in the browser

Actually, the main point of this blog post is not to show off "Dragonflight"; it's to keep
me from forgetting the magic formula for running DosBox in the browser! This is the same kind of
modern magic that [lets you play](https://archive.org/details/msdos_Invasion_of_the_Mutant_Space_Bats_of_Doom_1995)
_Invasion of the Mutant Space Bats of Doom_ over at the Internet Archive.
The Internet Archive apparently uses [em-dosbox](http://ascii.textfiles.com/archives/4924); but
I've found that [js-dos](https://js-dos.com) is much easier to set up, at least if your "build machine"
is not a Linux box, because js-dos comes pre-built.

To set up js-dos, you just place its three JS files ([js-dos.js](https://js-dos.com/6.22/current/js-dos.js),
[wdosbox.js](https://js-dos.com/6.22/current/wdosbox.js),
[wdosbox.wasm.js](https://js-dos.com/6.22/current/wdosbox.wasm.js))
together somewhere; and then make a tiny [index.html](/blog/code/2019-08-11-dflight.html)
that loads js-dos with [a .zip file](/blog/code/2019-08-11-dflight.zip) containing
the files you want on your DosBox's virtual disk plus instructions telling DosBox what command
it should run when the virtual machine starts up. And that's all! The only downside is that
wdosbox.wasm.js is a hefty 2.3MB in size. Fortunately, that's a one-time cost; you can have
many .html/.zip pairs all sharing the same .js machinery.

Nifty!
