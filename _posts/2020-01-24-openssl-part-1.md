---
layout: post
title: 'OpenSSL client and server from scratch, part 1'
date: 2020-01-24 00:01:00 +0000
tags:
  library-design
  networking
---

Lately I've been struggling through the quagmire that is OpenSSL, and now that I've got
some things working, I figure it's a good time to make a series of blog posts about them
for posterity.

This blog series consists of the following posts:

* [Part 1: HTTP client](/blog/2020/01/24/openssl-part-1/) (2020-01-24)
* [Part 2: HTTP server](/blog/2020/01/25/openssl-part-2/) (2020-01-25)
* [Part 3: HTTPS server](/blog/2020/01/26/openssl-part-3/) (2020-01-26)
* [Part 4: HTTPS client](/blog/2020/01/27/openssl-part-4/) (2020-01-27)
* [Part 5: HTTPS proxy client and server](/blog/2020/01/28/openssl-part-5/) (2020-01-28)


## Work with smart pointers by default

The OpenSSL library has a very object-oriented design, even though it's written
in plain old C. It's full of objects (`struct BIO`, `struct SSL`, `struct X509`)
which are "constructed" on the heap via `FOO_new`, "destroyed" via `FOO_free`,
and have a bunch of "methods" accessible via various `FOO_bar(foo, ...)` macros.
The first step in using OpenSSL from C++ should always be to wrap those objects
in RAII types so that you don't accidentally forget to `FOO_free` one of them
due to early return or `throw`.

If you watched my CppCon 2019 talk
["Back to Basics: Smart Pointers,"](https://www.youtube.com/watch?v=xGDLkt-jBJ4)
you already know how I'm going to do this.

    namespace my {

    template<class T> struct DeleterOf;
    template<> struct DeleterOf<SSL_CTX> { void operator()(SSL_CTX *p) const { SSL_CTX_free(p); } };
    template<> struct DeleterOf<SSL> { void operator()(SSL *p) const { SSL_free(p); } };
    template<> struct DeleterOf<BIO> { void operator()(BIO *p) const { BIO_free_all(p); } };
    template<> struct DeleterOf<BIO_METHOD> { void operator()(BIO_METHOD *p) const { BIO_meth_free(p); } };

    template<class OpenSSLType>
    using UniquePtr = std::unique_ptr<OpenSSLType, my::DeleterOf<OpenSSLType>>;

    } // namespace my

This simple framework immediately enables me to write things like

    auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));

    auto bio = my::UniquePtr<BIO>(BIO_new_connect("example.com:80"));
    if (BIO_do_connect(bio.get()) <= 0) {
        my::print_errors_and_exit("Error in BIO_do_connect");
    }

If you're going to be writing _real_ code manipulating these objects, I recommend going
a step further and creating your own `my::SSLContext`, `my::Connection`, and so on.
These `unique_ptr` aliases are still important and useful, because they serve as a "glue" layer
between raw `libopenssl` code and your own higher-level types.
See ["Back to Basics: Smart Pointers"](https://www.youtube.com/watch?v=xGDLkt-jBJ4&t=11m33s) for another example.
However, for this series of blog posts I'm going to stick with these `unique_ptr` aliases,
and forgo any higher-level business-logic types.


## A `BIO` that prints to a `std::string`

In the code above, I used a magic function called `my::print_errors_and_exit`. What does that look like?

    [[noreturn]] void print_errors_and_exit(const char *message)
    {
        fprintf(stderr, "%s\n", message);
        ERR_print_errors_fp(stderr);
        exit(1);
    }

OpenSSL provides a built-in function `ERR_print_errors_fp` that can dump the current error stack
to an arbitrary `FILE*`. So that's great. But what if we wanted to throw an exception instead of
just writing to `stderr`? It turns out that that's almost as simple, thanks to OpenSSL's built-in
function `ERR_print_errors`:

    [[noreturn]] void print_errors_and_throw(const char *message)
    {
        my::StringBIO bio;
        ERR_print_errors(bio.bio());
        throw std::runtime_error(std::string(message) + "\n" + std::move(bio).str());
    }

A `BIO`, in OpenSSL-speak, is sort of like a `FILE*` in C or a `std::iostream` in C++: it's a two-way
input/output channel. You can read from a `BIO` using the `BIO_read(bio, buf, size)` macro, and/or
write to it using the `BIO_write(bio, buf, size)` macro. If you're writing a network client or server,
you'll probably be reading and writing from a `BIO` that wraps a network socket.

There are two kinds of `BIO`s in OpenSSL: "source/sink BIOs" and "filter BIOs."
A source/sink BIO is like a network socket or a file descriptor: it's a source of data to be read,
and/or a sink for data that's written. Examples include [`BIO_s_file`](https://www.openssl.org/docs/man1.1.1/man3/BIO_s_file.html)
and [`BIO_s_socket`](https://www.openssl.org/docs/man1.1.1/man3/BIO_s_socket.html).
A filter BIO is an adaptor: it sits in front of some other BIO and filters its data in some way.
Examples include [`BIO_f_ssl`](https://www.openssl.org/docs/man1.1.1/man3/BIO_f_ssl.html)
and [`BIO_f_base64`](https://www.openssl.org/docs/man1.1.1/man3/BIO_f_base64.html).

How can we have so many different kinds of `BIO`s with only one type `struct BIO`?
Classical polymorphism! `struct BIO` itself is kind of like an "abstract base class."
Each method such as `BIO_read` looks up its behavior in a vtable of "BIO methods," a pointer to which
is stored within the `BIO` object itself. So if we want to make a new kind of `BIO` — a new "derived class"
in C++ terms — we do it by manually creating and filling in a new vtable. The C type of a BIO vtable
is `BIO_METHOD`.

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

Our C++ class `StringBIO` is not derived from `struct BIO`; it's actually an immovable wrapper that holds
together a `std::string` buffer and a pointer to our actual "derived class" BIO. We create a new "vtable"
for our BIO via `BIO_meth_new(BIO_TYPE_SOURCE_SINK, "StringBIO")` — the second argument is just an arbitrary
human-readable name for debugging, and the
[poorly documented](https://stackoverflow.com/questions/59883179/what-is-the-proper-use-of-bio-meth-new-bio-get-new-index)
first parameter just tells whether we're a source/sink BIO or a filter BIO.
We fill in the vtable entries that will be called by `ERR_print_errors`. We don't have to provide any
behavior for `BIO_read` — we can leave that one as "pure virtual" in C++ terms (which means anyone who
tries to call it will get an error). We fill in the entry for `BIO_write` by calling `BIO_meth_set_write`.

Notice my use of `UniquePtr<BIO_METHOD>` and `UniquePtr<BIO>`. The OpenSSL functions `BIO_meth_new` and
`BIO_new` return owning raw pointers, and I never want to touch them with my hands. They go straight into
their RAII wrappers.


## Reading and writing HTTP messages (slideware version)

Suppose I have a BIO that refers to a network socket. How do I send an HTTP request over that socket?
It's super easy!

    void send_http_request(BIO *bio, const std::string& line, const std::string& host)
    {
        std::string request = line + "\r\n";
        request += "Host: " + host + "\r\n";
        request += "\r\n";

        BIO_write(bio, request.data(), request.size());
        BIO_flush(bio);
    }

Now I can implement an HTTP client (no SSL yet):

    int main() {
        auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:80"));
        if (bio == nullptr) {
            my::print_errors_and_exit("Error in BIO_new_connect");
        }
        if (BIO_do_connect(bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_connect");
        }
        my::send_http_request(bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
        std::string response = my::receive_http_message(bio.get());
        printf("%s", response.c_str());
    }

The only complicated piece left to write is `my::receive_http_message`.
Receiving data over a socket is always tricky (as far as I know) because `recv`
(or `BIO_read` in our case) tends to block until the client sends some data, and
so it's important to know how much data you're expecting the client to send _before_
you actually try to read it. In the HTTP protocol, this catch-22 is handled by
looking for a `Content-Length:` header, which will tell us how many bytes to expect
in the message body.

(A non-slideware HTTP receiver would also have to deal with [chunked transfer encoding](https://en.wikipedia.org/wiki/Chunked_transfer_encoding)
and compressed/deflated/gzipped data, not to mention HTTP/2. I don't really know anything
about those things so I'm ignoring them.)

Our primitive operation here will be `my::receive_some_data(bio)`, which wraps a single call to `BIO_read`
and gives back a `std::string` with the next packet's worth of input (or throws, if unexpected stuff
happens).

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

To receive a single HTTP request, we first read packets until we find the end of the HTTP headers
(indicated by `\r\n\r\n`). Then we parse the `Content-Length:` header. Then we read packets until
we've read the expected number of bytes.

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

Notice that `split_headers` is extremely inefficient by C++ standards: a more C++ish interface
might accept a callback with signature `void cb(std::string_view)`, which we'd call for each
header — thus avoiding materialization of a bunch of `std::string`s as well as avoiding materialization
of a `std::vector` to hold them.


## Putting it all together

Here is the complete code for our very simple C++14 HTTP client. When you compile and run this code
with OpenSSL 1.1.0+, it should fetch the home page of [duckduckgo.com](https://duckduckgo.com)
over an unencrypted HTTP connection.

Godbolt Compiler Explorer doesn't support _running_ programs that do networking, but you can see the
code on Godbolt [here](https://godbolt.org/z/JkXUP4) anyway.

    // g++ -std=c++14 http-client.cpp $(pkg-config --cflags --libs openssl) -o http-client
    #include <memory>
    #include <stdexcept>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <string>
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

    void send_http_request(BIO *bio, const std::string& line, const std::string& host)
    {
        std::string request = line + "\r\n";
        request += "Host: " + host + "\r\n";
        request += "\r\n";

        BIO_write(bio, request.data(), request.size());
        BIO_flush(bio);
    }

    } // namespace my

    int main()
    {
        auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:80"));
        if (bio == nullptr) {
            my::print_errors_and_exit("Error in BIO_new_connect");
        }
        if (BIO_do_connect(bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_connect");
        }
        my::send_http_request(bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
        std::string response = my::receive_http_message(bio.get());
        printf("%s", response.c_str());
    }

This series continues with ["OpenSSL client and server from scratch, part 2."](/blog/2020/01/25/openssl-part-2/)
