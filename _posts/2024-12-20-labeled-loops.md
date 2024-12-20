---
layout: post
title: "Labeled loop syntax in many languages"
date: 2024-12-20 00:01:00 +0000
tags:
  esolang
  language-design
  proposal
  wg21
excerpt: |
  Many existing languages support labeled loops, such that you can say `break foo;` to break
  out of the loop _labeled_ `foo`, instead of just breaking from the smallest enclosing loop.
  The C programming language just gained labeled loops, too, with the adoption of
  [N3355 "Named loops"](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3355.htm) (Alex Celeste, 2024).

  Erich Keane and Aaron Ballman reacted to N3355's adoption with
  [N3377 "An Improved Syntax for N3355,"](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf)
  which I very much hope will be voted down. It's an "interesting" proposal, in that the authors
  are compiler engineers who work on a polyglot compiler (Clang), and yet they
  don't seem to consider consistency with other languages to be a positive for C at all.
---

{% raw %}
Many existing languages support labeled loops, such that you can say `break foo;` to break
out of the loop _labeled_ `foo`, instead of just breaking from the smallest enclosing loop.
The C programming language just gained labeled loops, too, with the adoption of
[N3355 "Named loops"](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3355.htm) (Alex Celeste, 2024).

Erich Keane and Aaron Ballman reacted to N3355's adoption with
[N3377 "An Improved Syntax for N3355,"](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf)
which I very much hope will be voted down. It's an "interesting" proposal, in that the authors
are compiler engineers who work on a polyglot compiler (Clang), and yet they
don't seem to consider consistency with other languages to be a positive for C at all.

Notably, Chris Lattner already implemented N3355's syntax
[_in Clang itself_](https://github.com/swiftlang/swift/commit/0a5b27baf463119e43842b9ea3b3f7d48e9b660e#diff-cd1d283f74e0b220c1ee25e926de1306d231805d50de3a4450856723173b2b85R565-R572)
back in 2014, for [Swift's](#swift) benefit. To be fair, mainline Clang's corresponding codepath
has over the past ten years diverged from Swift's (e.g. it's gone through a couple of renamings);
but there'd be no fundamental difficulty in upstreaming Lattner's patch.

Here, for the record, are all the languages I've found that support labeled loops today.

[Ada](#ada) — [Cpp2](#cpp2-cppfront) — [D](#d) — [Dart](#dart) — [Fortran](#fortran) — [Go](#go) —
[Java](#java) — [JavaScript](#javascript) – [Kotlin](#kotlin) — [Odin](#odin) —
[Perl](#perl) — [PL/I](#pli) — [PL/pgSQL](#plpgsql-postgresql) — [PowerShell](#powershell) —
[Rust](#rust) — [Swift](#swift)

> If you know any more languages, using <b>any</b> syntax — especially if you can find any language
> using syntax similar to N3377's! — please email me and let me know. Also please email me
> if you can confirm or correct the [PL/I](#pli) code below.

## Ada

([Godbolt.](https://godbolt.org/z/KoPfEbebb))

    OUTER:
    for x in 0..3 loop
      for y in 0..3 loop
        Put_Line(-(+"x=%d y=%d" & x & y));
        if A(x, y) /= 0 then
          exit OUTER;
        end if;
      end loop;
    end loop OUTER;
    Put_Line("Exited the outer loop");

## Cpp2 (Cppfront)

([Godbolt.](https://cpp2.godbolt.org/z/7oh56qdfr) Thanks to Vincent Hui for the CMake setup.)

    OUTER:
    for 0..<4 do (x) {
      for 0..<4 do (y) {
        printf("x=%d y=%d\n", x, y);
        if (A[x][y] != 0) {
          break OUTER;
        }
      }
    }
    puts("Exited the outer loop");

## D

([Godbolt.](https://godbolt.org/z/e9aczx5fh))

    OUTER:
    for (int x = 0; x < 4; ++x) {
      for (int y = 0; y < 4; ++y) {
        printf("x=%d y=%d\n", x, y);
        if (A[x][y] != 0) {
          break OUTER;
        }
      }
    }
    puts("Exited the outer loop");

## Dart

([Godbolt.](https://godbolt.org/z/zMnTzr1qK))

    OUTER:
    for (int x = 0; x < 4; ++x) {
      for (int y = 0; y < 4; ++y) {
        print("x=$x y=$y");
        if (A[x][y] != 0) {
          break OUTER;
        }
      }
    }
    print("Exited the outer loop");

## Fortran

([Godbolt.](https://godbolt.org/z/96frPa74Y) Thanks to Ivan Tubert-Brohman for this one!)

    OUTER: do x = 0, 3
      do y = 0, 3
        print '(''x='',i1,'' y='',i1)', x, y
        if (A(y, x) /= 0) then
          exit OUTER
        end if
      end do
    end do OUTER
    print '(A)', 'Exited the outer loop'

## Go

([Godbolt.](https://godbolt.org/z/3dfcedz6n))

    OUTER:
    for x := 0; x < 4; x++ {
      for y := 0; y < 4; y++ {
        fmt.Println("x=%d y=%d", x, y)
        if A[x][y] != 0 {
          break OUTER
        }
      }
    }
    fmt.Println("Exited the outer loop")

## Java

([Godbolt.](https://godbolt.org/z/h7Kbqb6sa))

    OUTER:
    for (int x = 0; x < 4; ++x) {
      for (int y = 0; y < 4; ++y) {
        System.out.printf("x=%d y=%d\n", x, y);
        if (A[x][y] != 0) {
          break OUTER;
        }
      }
    }
    System.out.printf("Exited the outer loop\n");

## JavaScript

([Godbolt.](https://godbolt.org/z/jGzjzn4ah))

    OUTER:
    for (let x = 0; x < 4; ++x) {
      for (let y = 0; y < 4; ++y) {
        console.log("x=" + x, "y=" + y);
        if (A[x][y] != 0) {
          break OUTER;
        }
      }
    }
    console.log("Exited the outer loop");

## Kotlin

([Godbolt.](https://godbolt.org/z/Ybzavcrnb))

    OUTER@
    for (x in 0..<4) {
      for (y in 0..<4) {
        println("x=%d y=%d".format(x, y))
        if (A[x][y] != 0) {
          break@OUTER
        }
      }
    }
    println("Exited the outer loop")

## Odin

([Godbolt.](https://godbolt.org/z/69KEYMMd9))

    OUTER:
    for x in 0..<4 {
      for y in 0..<4 {
        fmt.printf("x=%d y=%d\n", x, y)
        if A[x][y] != 0 {
          break OUTER
        }
      }
    }
    fmt.println("Exited the outer loop")

## Perl

([OneCompiler.](https://onecompiler.com/perl/433mg7fv7))

    OUTER:
    for (my $x = 0; $x < 4; ++$x) {
      for (my $y = 0; $y < 4; ++$y) {
        print "x=$x y=$y\n";
        if ($A[$x][$y] != 0) {
          last OUTER;
        }
      }
    }
    print "Exited the outer loop\n";

## PL/I

(I don't know any way to test PL/I. Corrections welcome.)

    main: proc options(main);
      declare A int(4,4)
        initial((9)0, 1, (5)0);
      OUTER:
      do x = 1 to 4;
        do y = 1 to 4;
          put list ('x=', x, ' y=', y)
          if A(x,y) ^= 0 then do;
            leave OUTER;
          end;
        end;
      end;
      put list('Exited the outer loop')
    end;

## PL/pgSQL (PostgreSQL)

(Copy the program below into [Crunchydata's Postgres Playground](https://www.crunchydata.com/developers/playground).)

    create or replace function main() returns void as $$
      declare
        A integer array;
      begin
        A = '{{0,0,0,0},{0,0,0,0},{0,1,0,0},{0,0,0,0}}';
        <<OUTER>>
        for x in 0..3 loop
          for y in 0..3 loop
            raise info 'x=% y=%', x, y;
            if A[x][y] then
              exit OUTER;
            end if;
          end loop;
        end loop;
        raise info 'Exited the outer loop';
      end;
    $$ language plpgsql;
    select main();

## PowerShell

([TIO.](https://tio.run/##bY1BC4JAEIXv8ysesgclBYNOiZAHz0HUSTwUrSgtrWwb7RL@9m3Vgg4yzAxv5nszvXxx9Wi5EM6xAjl2IcGXNJ4iihfVemFHUUZE2/3pWB6okQohM/5emsH3RGhsMqxWzER4e98M2C9g/wA7A0BgcmZi2JzZYBp0jbcUFTN1xWyN5M6R/mDgovj5hvn9qAcac6CgNJ3mV@iWQz41VxBS9gE59wE))

    :OUTER
    for ($x = 0; $x -lt 4; ++$x) {
      for ($y = 0; $y -lt 4; ++$y) {
        "x=$x y=$y"
        if ($A[$x][$y] -ne 0) {
          break OUTER
        }
      }
    }
    "Exited the outer loop"

## Rust

([Godbolt.](https://godbolt.org/z/5qT6a9K69))

    'OUTER:
    for x in 0..4 {
      for y in 0..4 {
        println!("x={} y={}", x, y);
        if A[x][y] != 0 {
          break 'OUTER;
        }
      }
    }
    println!("Exited the outer loop");

## Swift

([Godbolt.](https://godbolt.org/z/nzzGojPab))

    OUTER:
    for x in 0..<4 {
      for y in 0..<4 {
        print("x=\(x) y=\(y)");
        if (A[x][y] != 0) {
          break OUTER
        }
      }
    }
    print("Exited the outer loop")

## Languages without labeled loops

C# has a lot of folks asking for labeled loops over at [dotnet/csharplang#6634](https://github.com/dotnet/csharplang/discussions/6634),
but has never officially considered adding them as far as I know.

Python considered labeled loops in [PEP 3136](https://peps.python.org/pep-3136/), but the idea
was [rejected](https://mail.python.org/pipermail/python-3000/2007-July/008663.html) as both
unfocused (the PEP didn't propose _one_ syntax but rather brainstormed _five_ possible syntaxes)
and unnecessary.

Honestly, I don't think C or C++ need labeled loops either — I think they're
unnecessary — but if we _do_ get them, I hope it's with the well-thought-out
N3355 syntax shared by all the above modern languages, and not with
[N3377](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3377.pdf)'s
unprecedented and cumbersome syntax.
{% endraw %}
