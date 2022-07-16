---
layout: post
title: 'OpenSSL client and server from scratch, part 3'
date: 2020-01-26 00:01:00 +0000
tags:
  library-design
  networking
---

This is a continuation of yesterday's post,
["OpenSSL client and server from scratch, part 2."](/blog/2020/01/25/openssl-part-2/)
In the previous two posts, we made a trivial little HTTP client and a trivial little
HTTP server. Today we'll upgrade our server to use HTTP-over-TLS, a.k.a. HTTPS.


## `SSL_CTX` versus `SSL`

TLS is a stateful protocol. Each TLS connection needs to keep track of some connection-specific
state — like, what symmetric encryption algorithm we've agreed upon with the client, and
what keys we're using with it. This state is specific to the _connection_. OpenSSL stores it
in an object of type `struct SSL`. There are various `SSL_foo` macros to poke at the innards
of an `SSL` object, but for our purposes we don't need hardly any of them.

TLS is also a _complicated_ protocol with a lot of awkward human-scale inputs — like,
what encryption algorithms we consider acceptably secure, and what certificate we choose
to present to connecting clients, and what certificate authorities we trust. These inputs
tend to be shared across many individual TLS connections, so OpenSSL factors this state
out into an object of type `SSL_CTX`. Before you can create an `SSL` connection, you
need to create an `SSL_CTX` context.

    auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));

(This line uses the `my::UniquePtr` alias template from [part 1](/blog/2020/01/24/openssl-part-1/).)

[The documentation implies](https://www.openssl.org/docs/man1.1.1/man3/TLS_method.html)
that in real code you'd also want to prohibit [insecure and deprecated protocol versions](https://www.entrustdatacard.com/blog/2018/november/deprecating-tls)
by following that line up with something like

    SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);


## Caveats for OpenSSL 1.0.2

If you're using a version of OpenSSL prior to 1.1.0, you'll have to use `SSLv23_method()`
instead of `TLS_method()`. And `SSL_CTX_set_min_proto_version` won't exist, either.

This is also a good time to mention that OpenSSL 1.0.2 (unlike 1.1.0) will _not_ automatically initialize itself
the first time you use one of its facilities. Also, its error messages are cryptic integer codes by
default; if you want English text in your error messages, you must "load the error strings"
as part of your setup. That is, in 1.0.2 you need these lines at the top of `main()`:

    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
    #endif

I also should mention that OpenSSL 1.0.2 is missing a lot of macros for manipulating `BIO_METHOD`s.
You probably ran into this when you tried to use [part 1](/blog/2020/01/24/openssl-part-1/)'s `StringBIO`.
All the functionality is there in 1.0.2; what's missing are the macros to access it.
So what you need is a "polyfill" header for OpenSSL. The best one I've found is
["openssl_backport.h"](https://github.com/h2o/h2o/blob/master/include/h2o/openssl_backport.h),
part of the h2o project.

In short, OpenSSL 1.1.0 added a lot of minor conveniences over OpenSSL 1.0.2.
(And I should remark that even 1.1.0 was already end-of-lifed, in September 2019,
exactly one year after the release of OpenSSL 1.1.1. So if you're upgrading in 2020,
you should be upgrading to 1.1.1, not 1.1.0.)

We now return you to your regularly scheduled tiny HTTPS server.


## Set up the server's private key and certificate

When a TLS client connects to our server, it'll want to _authenticate_ that our server really is
who we say we are. Say the client is trying to talk to `duckduckgo.com` — is it _really_ talking to
`duckduckgo.com`, or is it talking to a malicious actor _posing_ as `duckduckgo.com`?
[On the Internet, nobody knows you're a dog.](https://en.wikipedia.org/wiki/On_the_Internet,_nobody_knows_you%27re_a_dog)

So how TLS solves this is, the client will demand that the server present a
[_certificate_](https://en.wikipedia.org/wiki/X.509) — a certificate of authenticity that
says "I certify that any server with public key such-and-such is authorized (by me) to serve data
for `duckduckgo.com`, at least until such-and-such a date. Signed, ...." And then it's signed
(cryptographically) by someone trustworthy — a _certificate authority_. Typically this is a big
company with a name like DigiCert, GlobalSign, or GeoTrust. Anyone who wants to talk
TLS must decide for themselves which certificate authorities they're going to trust as the "roots"
in their web of trust. [Here](https://curl.haxx.se/docs/caextract.html) is the list of CAs trusted
by Mozilla Firefox. If you want to run your own HTTPS website from scratch, and you want random
clients to trust that it is who it says it is, you must get a certificate for your site's domain name,
signed by one of these big root CAs (or signed by some "intermediate CA" who holds a slightly different
kind of certificate authorizing them to sign certificates, signed by some root CA; or so on).

By the way, when I said "any server with public key such-and-such," that was shorthand. What I meant
in full was: "any server which can prove that it knows the private half of the
[keypair](https://en.wikipedia.org/wiki/Public-key_cryptography) whose public half is such-and-such."

We'll use OpenSSL's command-line interface to generate a keypair using the P-256 elliptic curve algorithm.
(You can do this from C++ too, but that's out of scope for this post.)

    openssl ecparam -genkey -name prime256v1 -noout -out server-private-key.pem
    openssl ec -in server-private-key.pem -pubout -out server-public-key.pem

And then we'll create a certificate that says, "I certify that any server with public key
`server-public-key.pem` is authorized (by me) to serve data for `duckduckgo.com`, at least
for the next 30 days. Signed, `duckduckgo.com`." (And it's signed using `server-private-key.pem`.)

    openssl req -new -x509 -sha256 -key server-private-key.pem -subj "/CN=duckduckgo.com" -out server-certificate.pem

Notice that OpenSSL did not complain that we are not _actually_ `duckduckgo.com`; it's happy
to create the certificate for us. But if we present that certificate to someone out on the Web, they'll
check the signature and see that this certificate was _not_ signed by anyone they recognize as a CA,
so they probably won't trust what it says.
See also ["Portia's Caskets, Explained"](/blog/2018/07/06/portias-caskets-puzzle/) (2018-07-06).

Okay, so we have made our keypair and our server certificate. Back in the C++ code of our HTTPS server,
we must import these into our `SSL_CTX`. Unfortunately, OpenSSL doesn't provide any scalable or secure way
to import certificate or key data from memory; it wants everything as paths to disk files. Not the most
secure approach in the world; but suitable for our purposes.

    auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
    SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);
    if (SSL_CTX_use_certificate_file(ctx.get(), "server-certificate.pem", SSL_FILETYPE_PEM) <= 0) {
        my::print_errors_and_exit("Error loading server certificate");
    }
    if (SSL_CTX_use_PrivateKey_file(ctx.get(), "server-private-key.pem", SSL_FILETYPE_PEM) <= 0) {
        my::print_errors_and_exit("Error loading server private key");
    }


## Create an SSL filter BIO for each client connection

Our old server loop looked like this:

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

Our new TLS-enabled server loop looks like this:

    while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
        bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 0));
        try {
            std::string request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());
            my::send_http_response(bio.get(), "okay cool\n");
        } catch (const std::exception& ex) {
            printf("Worker exited with exception:\n%s\n", ex.what());
        }
    }

That's right — all we have to do is slap an SSL filter BIO in front of our socket BIO,
and we're good to go! OpenSSL's BIO API really makes this a cinch.

The one thing to really watch out for — and this bit me multiple times during the writing
of this series — is that integer `0` argument to `BIO_new_ssl`. It means "act like a server."
If you put a `1` there instead, it means "act like a client." The TLS protocol is not
symmetrical! If you write `0` when you mean `1`, or `1` when you mean `0`, your code will
probably just hang, or error out with some cryptic message if you're lucky. Be very careful
when cutting-and-pasting between examples!

> You might think that OpenSSL should provide macros `#define NEW_SSL_CLIENT 1` and
> `#define NEW_SSL_SERVER 0`.
> Yeah, it probably should! But it doesn't.


## Putting it all together, and testing

At the bottom of this post you'll find the complete code for our very simple C++14 HTTPS server.

As before, you can test the server program using [`curl`](https://linux.die.net/man/1/curl), like this:

    curl https://localhost:8080/

But look what happens when we run that line!

    $ curl https://localhost:8080/
    curl: (60) SSL certificate problem: self signed certificate
    More details here: https://curl.haxx.se/docs/sslcerts.html
    [...]

Aha! Our server is presenting a certificate that _is not trusted_ by `curl` (which is to say, it's
not trusted by the global "trust store" installed on our machine; on OSX, Apple stores trusted certs as
part of what it calls your "keychain"). Let's tell `curl` to expect that certificate and trust it:

    $ curl --cacert server-certificate.pem https://localhost:8080/
    curl: (51) SSL: certificate subject name 'duckduckgo.com' does not match target host name 'localhost'

Oh, right, we generated a certificate for `duckduckgo.com`. Let's tell `curl` to connect to `duckduckgo.com`...
but also tell it that for our purposes, `duckduckgo.com` lives right here on localhost.

    $ curl --cacert server-certificate.pem --resolve duckduckgo.com:8080:127.0.0.1 https://duckduckgo.com:8080/
    okay cool

Nifty!

----

Godbolt Compiler Explorer doesn’t support _running_ programs that do networking,
but you can see the code on Godbolt [here](https://godbolt.org/z/Zga7r7) anyway.

    // g++ -std=c++14 https-server.cpp $(pkg-config --cflags --libs openssl) -o https-server
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
    #include <openssl/ssl.h>

    namespace my {

    template<class T> struct DeleterOf;
    template<> struct DeleterOf<BIO> { void operator()(BIO *p) const { BIO_free_all(p); } };
    template<> struct DeleterOf<BIO_METHOD> { void operator()(BIO_METHOD *p) const { BIO_meth_free(p); } };
    template<> struct DeleterOf<SSL_CTX> { void operator()(SSL_CTX *p) const { SSL_CTX_free(p); } };

    template<class OpenSSLType>
    using UniquePtr = std::unique_ptr<OpenSSLType, DeleterOf<OpenSSLType>>;

    my::UniquePtr<BIO> operator|(my::UniquePtr<BIO> lower, my::UniquePtr<BIO> upper)
    {
        BIO_push(upper.get(), lower.release());
        return upper;
    }

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
    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
        auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_method()));
    #else
        auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
        SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);
    #endif

        if (SSL_CTX_use_certificate_file(ctx.get(), "server-certificate.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading server certificate");
        }
        if (SSL_CTX_use_PrivateKey_file(ctx.get(), "server-private-key.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading server private key");
        }

        auto accept_bio = my::UniquePtr<BIO>(BIO_new_accept("8080"));
        if (BIO_do_accept(accept_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_accept (binding to port 8080)");
        }

        static auto shutdown_the_socket = [fd = BIO_get_fd(accept_bio.get(), nullptr)]() {
            close(fd);
        };
        signal(SIGINT, [](int) { shutdown_the_socket(); });

        while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
            bio = std::move(bio)
                | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 0))
                ;
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

This series continues with ["OpenSSL client and server from scratch, part 4."](/blog/2020/01/27/openssl-part-4/)
