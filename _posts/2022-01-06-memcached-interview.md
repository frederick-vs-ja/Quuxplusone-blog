---
layout: post
title: "The best engineering interview question I've ever gotten, Part 1"
date: 2022-01-06 00:01:00 +0000
tags:
  sre
  war-stories
---

It's been a while since I was on the receiving end of a software engineering
interview. But I still remember my favorite interview question. It was at MemSQL circa 2013.
(They [haven't even kept their name](https://www.singlestore.com/blog/revolution/),
so I assume they're not still relying on this specific interview question.
I don't feel bad for revealing it. It's a great story that I tell people a lot;
I've just never blogged it before.)

Okay, so this isn't a "question" per se; it's a programming challenge.
I forget how much time they gave for it. Let's say three hours, counting
the time to explain the problem. [UPDATE, 2024: I'm reliably informed that
it was only <a href="https://news.ycombinator.com/item?id=31065845">one hour</a>,
not three; adjust the estimates below accordingly.]

Since MemSQL was a database company, this is a database challenge.


## Introducing memcached

You know about `memcached`? No? Well, it's an in-memory key-value store.
([Read its About page here.](https://memcached.org/about)) Let's download
its code and build it so I can show you what it does.

> You may need to `brew install libevent` and maybe some other
> stuff in order to build memcached successfully. It won't be too
> difficult to figure out; but anyway, wrangling with your environment
> wasn't part of the test. You can assume interviewees were given
> access to a Linux box with all the right dependencies in place already.

For the authentic 2013 experience, let's bypass the [GitHub repo](https://github.com/memcached/memcached)
and untar a contemporary source distribution:

    curl -O https://memcached.org/files/memcached-1.4.15.tar.gz
    tar zxvf memcached-1.4.15.tar.gz
    cd memcached-1.4.15
    ./configure
    make

Now we've built the `memcached` executable. Let's start it running:

    ./memcached

We can talk to the server via the default memcached port, port 11211.
Its protocol is basically plain text, so we can use plain old `telnet`
to talk to it. (If you don't have `telnet` anymore, substitute the
words [`nc -c`](https://www.unixfu.ch/use-netcat-instead-of-telnet/) for `telnet`.)

    $ telnet 127.0.0.1 11211
    Trying 127.0.0.1...
    Connected to localhost.
    Escape character is '^]'.


## Playing with memcached

memcached is a key-value store. That means we can tell it to remember
something (an association between a key and a value), and then later ask for
that key again, and it'll tell us the value it remembered. In memcached,
keys are always ASCII strings and values are always arbitrary byte streams
(which means you must specify their length manually). For example,
type this into your telnet session:

    set fullname 0 3600 10
    John Smith

This tells memcached to remember an association between the string key `fullname`
and the 10-byte value `John Smith`. The other numbers on that line are a "flags" value
(`0`) to remember alongside the byte-stream value; and an expiry timeout (`3600`)
measured in seconds, after which memcached will forget this association.
Anyway, after you type these two lines, memcached will respond:

    STORED

Now you can retrieve the value of `fullname` by typing into the same telnet
session:

    get fullname

memcached will respond:

    VALUE fullname 0 10
    John Smith
    END

You can overwrite the value associated with `fullname` by issuing another
`set fullname` command. You can also ask memcached to modify the value in certain
ways; for example, there are special dedicated commands for `append` and `prepend`.

    append fullname 0 3600 6
    -Jones
    STORED
    get fullname
    VALUE fullname 0 16
    John Smith-Jones
    END

Of course if you wanted to append `-Jones` to `fullname` from within a client program,
you _could_ do something like this:

    # pip install python-memcached
    import memcache
    mc = memcache.Client(['127.0.0.1:11211'])
    v = mc.get('fullname')    # get the old value from memcached
    v += '-Jones'             # append -Jones
    mc.set('fullname', v)     # set the new value into memcached

But the advantage of memcached's dedicated `append` command is that it's guaranteed
to execute _atomically_. If you have multiple clients connected to the same memcached
server, and they're all trying to append to the same key at the same time, the
`get/set` version might cause some of those updates to be lost, whereas with `append`
you're assured they'll all be accounted for.

Another dedicated command that executes atomically is `incr`:

    set age 0 3600 2
    37
    STORED
    incr age 1

memcached responds with the postincremented value:

    38

This response is useful because of the multiple clients thing. If you issued a separate
`get age` command, you might see the new value only after several other clients had
done their own updates. If you're intending to use the value as a serial number,
or a SQL primary key, or something like that, then it's very good that there's a
way to see the incremented value _atomically_.

memcached remembers the incremented value too, of course:

    get age
    VALUE age 0 2
    38
    END

Notice that `37` and `38` are still being stored and returned as byte-strings;
they're decoded from ASCII into integers and back as part of the atomic operation.
If you try to `incr` a non-integer value, you get an error:

    incr fullname 1
    CLIENT_ERROR cannot increment or decrement non-numeric value

Finally, note that `incr` is a full-fledged addition operation: you can increment
(or `decr`) by any positive value, not just by `1`.

    incr age 10
    48
    decr age 10
    38
    incr age -1
    CLIENT_ERROR invalid numeric delta argument

By the way, when you're done talking to memcached and want to kill the connection,
you can type the memcached command `quit`. (If you're using `nc -c`, Ctrl+D also
works. Or, just go to the terminal window where the `memcached` server is running
and Ctrl+C to bring it down.)


## The challenge: Modifying memcached

Via its `incr` and `decr` commands, memcached provides a built-in way to
atomically add $$k$$ to a number. But it doesn't provide other arithmetic
operations; in particular, there is no "atomic multiply by $$k$$" operation.

Your programming challenge: <b>Add a `mult` command to memcached.</b>

When you're done, I should be able to telnet to your memcached client
and run commands like

    mult age 10
    380

You have three hours. Go!

----

For spoilers and analysis, see
["The best engineering interview question I've ever gotten, Part 2."](/blog/2022/01/07/memcached-interview-solution/)
