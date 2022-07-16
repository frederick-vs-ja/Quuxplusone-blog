---
layout: post
title: 'OpenSSL client and server from scratch, part 2'
date: 2020-01-25 00:01:00 +0000
tags:
  library-design
  networking
---

This is a continuation of yesterday's post,
["OpenSSL client and server from scratch, part 1."](/blog/2020/01/24/openssl-part-1/)
Today we'll be building a trivial little HTTP server.


## BIO chains, pushing and popping

Yesterday we talked about how BIOs come in two flavors: "filter BIOs" and "source/sink BIOs."
A "BIO chain" consists of a source/sink BIO preceded by zero or more filter BIOs. When we have
a BIO chain, we work exclusively with the BIO on the front of the chain. That BIO talks to its
"next" BIO, which talks to _its_ next BIO, and so on, all the way down to the source (and/or sink).
Think of the first BIO in a chain as the "top" BIO in a stack of BIOs.

OpenSSL manipulates this stack of BIOs using the aptly named macros
[`BIO_push` and `BIO_pop`](https://www.openssl.org/docs/man1.1.1/man3/BIO_push.html).
Unfortunately, the documentation is again just awful:

>     BIO *BIO_push(BIO *b, BIO *append);
>     BIO *BIO_pop(BIO *b);
>
> The BIO_push() function appends the BIO `append` to `b`, it returns `b`.
> BIO_pop() removes the BIO `b` from a chain and returns the next BIO in the chain,
> or NULL if there is no next BIO. The removed BIO then becomes a single BIO with
> no association with the original chain.

To clarify the semantics, let's overload a Ranges-style `operator|`.

    my::UniquePtr<BIO> operator|(my::UniquePtr<BIO> lower,
                                 my::UniquePtr<BIO> upper)
    {
        BIO_push(upper.get(), lower.release());
        return upper;
    }

Now we can write something like the following, and it has the "obvious" meaning:
take the socket BIO we started with, and filter it through an SSL filter BIO.

    auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:443"));
    if (BIO_do_connect(bio.get()) <= 0) {
        my::print_errors_and_exit("Error in BIO_do_connect");
    }
    bio = std::move(bio) | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 1));

After the assignment on the last line above, `bio` points to an SSL filter BIO,
and the SSL filter BIO's "next BIO" is the source/sink BIO talking
TCP to duckduckgo.com. Anything we `BIO_read` or `BIO_write` into `bio` will
be encrypted with TLS and then passed along to duckduckgo.com. (Very much handwaving
here. The point is that we have a BIO chain, and the front of the chain is the
SSL filter BIO.)

But we're not going to talk about TLS yet. Let's talk about `BIO_pop` first.


## `BIO_s_accept`'s weird relationship with `BIO_pop`

When we wrote our HTTP client in part 1, we used `BIO_new_connect` to create a
client connection to a remote server. Now that we're writing an HTTP server,
we'll use `BIO_new_accept` to create a... well, _not_ to create a connection serving
a remote client. We don't want to create just one connection. We want a _factory_
that listens on a dedicated port and repeatedly creates connections from remote clients.

The way OpenSSL does this is kind of wacky. You and I know that a factory for
creating connections is not _itself_ a connection. But in OpenSSL's world, both
of these things are represented as `BIO` objects. We have an "accept BIO" that
represents a factory. We call the function `BIO_do_accept(accept_bio)` on this factory.
It blocks until a client tries to connect to the server, and then it produces a
new socket BIO to represent that connection — and the _way_ it "produces" the new
BIO is surprising! After a successful call to `BIO_do_accept(accept_bio)`,
you'll find that
[a brand-new socket BIO has been inserted](https://github.com/openssl/openssl/blob/3d48457478bd61030c370e4090c1462fc4453d81/crypto/bio/bss_acpt.c#L303-L322)
right behind `accept_bio` in its BIO chain. (The BIO chain of an accept BIO should
never have multiple BIOs in it.)

So when we wrap this weird factory API in a semi-sensible C++ function, we end up
with something like this:

    my::UniquePtr<BIO> accept_new_tcp_connection(BIO *accept_bio)
    {
        if (BIO_do_accept(accept_bio) <= 0) {
            return nullptr;
        }
        return my::UniquePtr<BIO>(BIO_pop(accept_bio));
    }

    int main()
    {
        auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
        if (BIO_do_accept(accept_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
        }

        while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
            try {
                std::string request = my::receive_http_message(bio.get());
                printf("Got request:\n");
                printf("%s\n", request.c_str());
                my::send_http_response(bio.get(), "okay cool\n");
            } catch (const std::exception& ex) {
                printf("Worker exited with exception:\n%s\n", ex.what());
            }
        }
    }

Notice that we make _two_ calls to `BIO_do_accept`, not just one call! The reason for this
is that all three of `BIO_do_accept`, `BIO_do_connect`, and `BIO_do_handshake` are simple macro
synonyms for `BIO_ctrl(b, BIO_C_DO_STATE_MACHINE, 0, NULL)`. Accept, connect, and SSL BIOs
are state machines, and the behavior of `BIO_do_accept` (or `BIO_do_handshake`) depends on
the current state of the state machine. If the accept BIO has not yet bound to a port, then
`BIO_do_accept` will bind (but not block yet). Once it's bound to a port, a second call to
`BIO_do_accept` will listen on that port (blocking until a connection comes in, or until the socket is closed).

Also notice that `accept_new_tcp_connection(BIO *accept_bio)` takes a raw `BIO*`. In this series
of posts, we consistently use smart pointers to represent and transfer ownership. So, when
you see a raw pointer like this, you should immediately infer that ownership of the
original `accept_bio` is _not_ being transferred here. We don't care who owns the accept BIO;
we manipulate it but do not participate in its ownership. On the other hand, you can tell from
the return type `my::UniquePtr<BIO>` that we are transferring ownership of the newly created
socket BIO back to our caller — ownership of the new socket BIO becomes the caller's responsibility.

The only remaining piece of our toy HTTP server is `my::send_http_response`, which is just
about as trivial as `my::send_http_request` from yesterday's post.

    void send_http_response(BIO *bio, const std::string& body)
    {
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";

        BIO_write(bio, response.data(), response.size());
        BIO_write(bio, body.data(), body.size());
        BIO_flush(bio);
    }


## Cleanly shutting down the server

Notice that our server's main loop runs until `accept_new_tcp_connection` returns null,
which it never does under normal circumstances. If you Ctrl-C the server, it exits suddenly
without any chance for custom cleanup. I don't really know the best way to deal with this,
but one way that seems to work for me in practice is to set up a SIGINT handler. When you
hit Ctrl-C, it generates a SIGINT signal; our SIGINT handler closes the accept BIO's socket;
the accept BIO notices this and returns null from `BIO_do_accept`; and then we can go on
and take appropriate action. Here's what our `main()` looks like if we do that:

    int main()
    {
        auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
        if (BIO_do_accept(accept_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
        }

        static auto shutdown_the_socket = [fd = BIO_get_fd(accept_bio.get(), nullptr)]() {
            close(fd);
        };
        signal(SIGINT, [](int) { shutdown_the_socket(); });

        while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
            try {
                std::string request = my::receive_http_message(bio.get());
                printf("Got request:\n");
                printf("%s\n", request.c_str());
                my::send_http_response(bio.get(), "okay cool\n");
            } catch (const std::exception& ex) {
                printf("Worker exited with exception:\n%s\n", ex.what());
            }
        }
        printf("\nClean exit!\n");
    }

Notice how our signal handler is a captureless lambda (which, being captureless, decays to
the function pointer expected by `signal`). So how can it refer to the local variable `shutdown_the_socket`?
Easy: `shutdown_the_socket` is a variable with static lifetime, so we don't need to capture a copy
of it — we can refer to it from within a captureless lambda just as easily as we can refer to a
global variable or a free function. (If you've seen my "Lambdas from Scratch" talk, you'll recognize
this as a version of [the `kitten`/`cat` puzzle](https://www.youtube.com/watch?v=3jCOwajNch0&t=20m29s).)

Again, I don't claim that this is a _good_ way for a long-running server to handle shutdown,
but it works well enough for my slideware.

See the list of signal-safe functions here: [`man signal-safety`](http://man7.org/linux/man-pages/man7/signal-safety.7.html).


## Putting it all together

Here is the complete code for our very simple C++14 HTTP server.
When you compile and run this code with OpenSSL 1.1.0+, it should run forever (or until
you kill it), listening on port 8080 for unencrypted HTTP requests.
It responds blindly to every request with `okay cool\n`.

You can test this code by using the very simple HTTP client from yesterday's post;
you just have to change the first line of that client's `main()` from

    auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:80"));

to

    auto bio = my::UniquePtr<BIO>(BIO_new_connect("localhost:8080"));

Another way to test the server program is to use the command-line utility [`curl`](https://linux.die.net/man/1/curl):

    $ curl http://localhost:8080/
    okay cool

Godbolt Compiler Explorer doesn’t support _running_ programs that do networking,
but you can see the code on Godbolt [here](https://godbolt.org/z/DGjD5L) anyway.

    // g++ -std=c++14 http-server.cpp $(pkg-config --cflags --libs openssl) -o http-server
    #include <memory>
    #include <signal.h>
    #include <stdexcept>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <string>
    #include <unistd.h>
    #include <vector>

    #include <openssl/bio.h>
    #include <openssl/err.h>

    namespace my {

    template<class T> struct DeleterOf;
    template<> struct DeleterOf<BIO> { void operator()(BIO *p) const { BIO_free_all(p); } };
    template<> struct DeleterOf<BIO_METHOD> { void operator()(BIO_METHOD *p) const { BIO_meth_free(p); } };

    template<class OpenSSLType>
    using UniquePtr = std::unique_ptr<OpenSSLType, DeleterOf<OpenSSLType>>;

    class StringBIO {
        std::string str_;
        my::UniquePtr<BIO_METHOD> methods_;
        my::UniquePtr<BIO> bio_;
    public:
        StringBIO(StringBIO&&) = delete;
        StringBIO& operator=(StringBIO&&) = delete;

        explicit StringBIO() {
            methods_.reset(BIO_meth_new(BIO_TYPE_SOURCE_SINK, "StringBIO"));
            if (methods_ == nullptr) {
                throw std::runtime_error("StringBIO: error in BIO_meth_new");
            }
            BIO_meth_set_write(methods_.get(), [](BIO *bio, const char *data, int len) -> int {
                std::string *str = reinterpret_cast<std::string*>(BIO_get_data(bio));
                str->append(data, len);
                return len;
            });
            bio_.reset(BIO_new(methods_.get()));
            if (bio_ == nullptr) {
                throw std::runtime_error("StringBIO: error in BIO_new");
            }
            BIO_set_data(bio_.get(), &str_);
            BIO_set_init(bio_.get(), 1);
        }
        BIO *bio() { return bio_.get(); }
        std::string str() && { return std::move(str_); }
    };

    [[noreturn]] void print_errors_and_exit(const char *message)
    {
        fprintf(stderr, "%s\n", message);
        ERR_print_errors_fp(stderr);
        exit(1);
    }

    [[noreturn]] void print_errors_and_throw(const char *message)
    {
        my::StringBIO bio;
        ERR_print_errors(bio.bio());
        throw std::runtime_error(std::string(message) + "\n" + std::move(bio).str());
    }

    std::string receive_some_data(BIO *bio)
    {
        char buffer[1024];
        int len = BIO_read(bio, buffer, sizeof(buffer));
        if (len < 0) {
            my::print_errors_and_throw("error in BIO_read");
        } else if (len > 0) {
            return std::string(buffer, len);
        } else if (BIO_should_retry(bio)) {
            return receive_some_data(bio);
        } else {
            my::print_errors_and_throw("empty BIO_read");
        }
    }

    std::vector<std::string> split_headers(const std::string& text)
    {
        std::vector<std::string> lines;
        const char *start = text.c_str();
        while (const char *end = strstr(start, "\r\n")) {
            lines.push_back(std::string(start, end));
            start = end + 2;
        }
        return lines;
    }

    std::string receive_http_message(BIO *bio)
    {
        std::string headers = my::receive_some_data(bio);
        char *end_of_headers = strstr(&headers[0], "\r\n\r\n");
        while (end_of_headers == nullptr) {
            headers += my::receive_some_data(bio);
            end_of_headers = strstr(&headers[0], "\r\n\r\n");
        }
        std::string body = std::string(end_of_headers+4, &headers[headers.size()]);
        headers.resize(end_of_headers+2 - &headers[0]);
        size_t content_length = 0;
        for (const std::string& line : my::split_headers(headers)) {
            if (const char *colon = strchr(line.c_str(), ':')) {
                auto header_name = std::string(&line[0], colon);
                if (header_name == "Content-Length") {
                    content_length = std::stoul(colon+1);
                }
            }
        }
        while (body.size() < content_length) {
            body += my::receive_some_data(bio);
        }
        return headers + "\r\n" + body;
    }

    void send_http_response(BIO *bio, const std::string& body)
    {
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Length: " + std::to_string(body.size()) + "\r\n";
        response += "\r\n";

        BIO_write(bio, response.data(), response.size());
        BIO_write(bio, body.data(), body.size());
        BIO_flush(bio);
    }

    my::UniquePtr<BIO> accept_new_tcp_connection(BIO *accept_bio)
    {
        if (BIO_do_accept(accept_bio) <= 0) {
            return nullptr;
        }
        return my::UniquePtr<BIO>(BIO_pop(accept_bio));
    }

    } // namespace my

    int main()
    {
        auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
        if (BIO_do_accept(accept_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
        }

        static auto shutdown_the_socket = [fd = BIO_get_fd(accept_bio.get(), nullptr)]() {
            close(fd);
        };
        signal(SIGINT, [](int) { shutdown_the_socket(); });

        while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
            try {
                std::string request = my::receive_http_message(bio.get());
                printf("Got request:\n");
                printf("%s\n", request.c_str());
                my::send_http_response(bio.get(), "okay cool\n");
            } catch (const std::exception& ex) {
                printf("Worker exited with exception:\n%s\n", ex.what());
            }
        }
        printf("\nClean exit!\n");
    }

This series continues with ["OpenSSL client and server from scratch, part 3."](/blog/2020/01/26/openssl-part-3/)
