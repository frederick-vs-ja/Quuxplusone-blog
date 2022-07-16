---
layout: post
title: 'OpenSSL client and server from scratch, part 4'
date: 2020-01-27 00:01:00 +0000
tags:
  library-design
  networking
---

This is a continuation of yesterday's post,
["OpenSSL client and server from scratch, part 3."](/blog/2020/01/26/openssl-part-3/)
In the previous post, we made a trivial little HTTPS server that we could talk to with `curl`.
Today we'll write our own HTTPS client as a replacement for `curl`.


## Set up an `SSL_CTX` for the client

Recall that before we can create an `SSL` connection, we need to fill out
an `SSL_CTX`. On the server side, the `SSL_CTX` holds the server's certificate and private key,
so that the server can authenticate itself to clients. On the client side, the `SSL_CTX` holds
a _trust store_ — a set of certificates that our client considers trustworthy.

> By the way, it is also possible for the _client_ to present a certificate and for the _server_
> to do certificate verification. This is extremely uncommon on the World Wide Web,
> where servers offer their services mostly to strangers, but you may have encountered client
> authentication on a private network. For example, your browser might need to present
> a "client certificate" as part of logging in to your employer's email service.
>
> We won't talk about client certificates today.

If you don't set up a trust store, OpenSSL won't trust any certificates by default.
That's not what you want! So let's tell OpenSSL to trust "whatever our computer trusts,"
just like `curl` probably does. So we create our `SSL_CTX` object and then call
`SSL_CTX_set_default_verify_paths`:

    auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
    SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION);

    if (SSL_CTX_set_default_verify_paths(ctx.get()) != 1) {
        my::print_errors_and_exit("Error loading trust store");
    }

> You can locate your default trust store using this OpenSSL function:
>
>     puts(X509_get_default_cert_file());
>
> Or by running this Python command:
>
>     python -c 'import ssl; print(ssl.get_default_verify_paths().cafile)'

Now we set up our TCP connection to `duckduckgo.com`, just like we did in [part 1](/blog/2020/01/24/openssl-part-1/).
The only difference is that we connect to port 443 (the port conventionally associated with HTTPS services)
instead of to port 80.

    auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:443"));
    if (BIO_do_connect(bio.get()) <= 0) {
        my::print_errors_and_exit("Error in BIO_do_connect");
    }

Once we've made the TCP connection, it's time to start talking TLS.
We slap an SSL filter BIO in front of our socket BIO, using the overloaded `operator|`
that we wrote in [part 2](/blog/2020/01/25/openssl-part-2/).

    auto ssl_bio = std::move(bio)
        | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 1));

Notice the integer argument `1` here. Recall from [part 3](/blog/2020/01/26/openssl-part-3/) that
`1` means "I'm a client," and `0` means "I'm a server." This line of code is just one character
different from what we wrote yesterday, but if you cut-and-paste from yesterday's code and forget
to change that one character, you'll be in a world of cryptic and frustrating errors!


## Getting at the actual `SSL` object

Our SSL filter BIO owns an `SSL` object, which holds the state of the actual connection
(like, what symmetric encryption key it's negotiated, or what certificate it has received from the server),
as distinct from the SSL "context" `SSL_CTX` that we already set up. (By the way, the `SSL` object
also holds a reference to our `SSL_CTX`; `SSL_CTX` objects, like most OpenSSL objects, are refcounted
internally. So we could actually relinquish _our_ ownership by setting `ctx = nullptr` at this point,
and it would do no harm. The `SSL_CTX` object won't actually disappear from memory until _every_ stakeholder
is done with it.)

We're about to do some work with the per-connection `SSL` object,
so let's write a quick helper function to get at it:

    SSL *get_ssl(BIO *bio)
    {
        SSL *ssl = nullptr;
        BIO_get_ssl(bio, &ssl);
        if (ssl == nullptr) {
            my::print_errors_and_exit("Error in BIO_get_ssl");
        }
        return ssl;
    }

Keep in mind that we always read and write data via a BIO. The `BIO` object is higher-level
than the `SSL` object. OpenSSL does provide lower-level macros like `SSL_read` and `SSL_write` that talk
directly to the `SSL` object, but we aren't going to use them. The only time we'll touch an `SSL` object
is when we have absolutely no other choice.


## The SNI field

You're reading this blog post on [quuxplusone.github.io](https://quuxplusone.github.io/blog/),
which means that the HTTPS server serving you this page must present a certificate for `quuxplusone.github.io`.
But the server serving you this page (let's say its IP address is 185.199.108.153)
probably serves dozens or hundreds of other domains, too. (And not just `*.github.io` domains, either,
so wildcard certificates don't fully solve the problem, smarty pants.)
How does the server know what certificate to present to you?

(Remember, it can't look at the `Host:` header of your HTTP request, because you haven't sent
your HTTP request yet, because you can't be sure if you trust this server until it's presented you
with a certificate.)

TLS solves this chicken-and-egg problem by encouraging the client to
send their destination domain in plaintext as part of the TLS handshake. It goes in a field
called "Server Name Indication" (SNI), as specified in [RFC 6066](https://tools.ietf.org/html/rfc6066#section-3).
OpenSSL does not set the SNI field by default, but in real life you _should_ set it, like this:

    SSL_set_tlsext_host_name(my::get_ssl(ssl_bio.get()), "duckduckgo.com");


## The TLS handshake and certificate verification

When it comes to Internet security,
[the server is involved; the client is committed.](https://english.stackexchange.com/questions/188455/)

Our trivial HTTPS server merely presents its certificate to the client and waits to see if the client
accepts it. This is a non-interactive process from the programmer's point of view. We just
set up the server's `SSL_CTX` with the appropriate certificate and private key, and then we could
immediately call `BIO_read` on the SSL BIO; the process of presenting our certificate and negotiating
encryption parameters all happened "under the hood." If the client happens to reject our certificate,
no problem — `BIO_read` returns zero bytes, we drop the connection and move on to service the
next client.

Our trivial HTTPS client has to do more work. Before we do anything else with its SSL connection,
we need to verify that the server is who they say they are. So _before we issue our first `BIO_write`
on the connection_, let's complete the TLS handshake: let's receive the server's certificate, check whether it
has a valid chain of trust back to a root in our trust store, and check whether any of the links in
that chain have expired. Only if the verification succeeds should we go on to send our GET request
over the encrypted connection.

    if (BIO_do_handshake(ssl_bio.get()) <= 0) {
        my::print_errors_and_exit("Error in TLS handshake");
    }
    my::verify_the_certificate(my::get_ssl(ssl_bio.get()), "duckduckgo.com");
    my::send_http_request(ssl_bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
    std::string response = my::receive_http_message(ssl_bio.get());
    printf("%s", response.c_str());

Recall from [part 2](/blog/2020/01/25/openssl-part-2/) that `BIO_do_handshake` is just a macro that
means "do the next step in whatever this BIO's state machine is," and that functionally it's a
synonym for `BIO_do_connect` and `BIO_do_accept`. We _could_ write

    if (BIO_do_connect(ssl_bio.get()) <= 0) {
        my::print_errors_and_exit("Error in TLS handshake");
    }

and the compiler would treat it as exactly the same thing... but we shouldn't! The point of this
line of code is to complete a TLS handshake, _not_ to make a TCP connection to a server
nor to accept a TCP connection from a client. So we should use the `BIO_do_handshake` mnemonic
to convey our intent to the human reader.

Let's look at the implementation of `my::verify_the_certificate`.

    void verify_the_certificate(SSL *ssl, const std::string& expected_hostname)
    {
        int err = SSL_get_verify_result(ssl);
        if (err != X509_V_OK) {
            const char *message = X509_verify_cert_error_string(err);
            fprintf(stderr, "Certificate verification error: %s (%d)\n", message, err);
            exit(1);
        }
        X509 *cert = SSL_get_peer_certificate(ssl);
        if (cert == nullptr) {
            fprintf(stderr, "No certificate was presented by the server\n");
            exit(1);
        }
        if (X509_check_host(cert, expected_hostname.data(), expected_hostname.size(), 0, nullptr) != 1) {
            fprintf(stderr, "Certificate verification error: Hostname mismatch\n");
            exit(1);
        }
    }

There are three things we need to check in order to verify the server's certificate.

- First, ask OpenSSL whether there was anything "off" about the certificate presented by the server.
    Did the server present us with an expired certificate? Was the certificate not signed by anyone
    we trust? (Remember, the `SSL` object knows about the `SSL_CTX`, and we set up the `SSL_CTX`
    with a trust store.) Did the server fail to prove that it knows the private key associated with
    the certificate?

- Second, check that the server actually provided us with a certificate! The TLS protocol doesn't
    technically require the server to present a certificate, any more than it requires the client to
    present one. If no certificate was presented, then OpenSSL will happily report "nothing was wrong
    with the certificate [because there wasn't one]." Therefore it's extremely important that we do
    this step!

- Finally, remember how SNI was kind of an afterthought? Suppose the server presents us with a valid
    certificate for `google.com`, signed by someone we trust, and proves that it knows the private key
    associated with the certificate. This is absolutely sufficient evidence that the server we're talking
    to _is really_ `google.com`. But that could still be a problem — if we _wanted_ to talk to `duckduckgo.com`!
    Therefore our third step is to verify that the name on the certificate matches the name of the server
    that we asked to talk to.

A single certificate may be valid for many different domain names. One of those names occupies a
privileged position known as the "Common Name" (CN); the rest must be relegated to an extension field called
"Subject Alternative Names" (SAN). However, because storing names in two different places is a pain in the neck,
the industry has essentially
[deprecated](https://security.stackexchange.com/questions/69156/using-commonname-for-dns-vs-subjaltname)
the "Common Name" field in favor of putting _all_ the names in the SAN field.

When we generated our `duckduckgo.com` certificate in [part 3](/blog/2020/01/26/openssl-part-3/), we used
that deprecated CN field:

    openssl req -new -x509 -sha256 -key server-private-key.pem -subj "/CN=duckduckgo.com" -out server-certificate.pem

To generate a certificate with SANs like all the cool kids have,
[consult StackOverflow](https://security.stackexchange.com/questions/74345/provide-subjectaltname-to-openssl-directly-on-the-command-line);
it seems to be very difficult pre-1.1.1 and maybe still kind of tricky even in 1.1.1, judging from the
various answers.

Anyway, the point is, when we do that final step of checking that the cert presented by the server
was issued for the domain that we're actually trying to reach, we won't be checking "the name"
on the certificate; we'll be checking the _names_ on the certificate, plural, to see if _any_ of them
match the domain we're trying to reach. We should also consider wildcards; for example, a certificate issued for
`*.github.io` should match `quuxplusone.github.io`. This logic is extremely tricky, so we will
let OpenSSL do it for us, by calling `X509_check_host` as shown above.

OpenSSL 1.1.0 can do `X509_check_host` as part of `SSL_get_verify_result`... if we add a setup step
that tells OpenSSL to do the check for us! The setup step is spelled
[`SSL_set1_host`](https://www.openssl.org/docs/man1.1.0/man3/SSL_set1_host.html), and you can see it in
the complete code below.

> Prior to OpenSSL 1.0.2, the function `X509_check_host` didn't even exist, and everyone had to roll their own
> implementations. A high-quality, but limited-functionality, example of hostname validation is at
> [iSECPartners/ssl-conservatory](https://github.com/iSECPartners/ssl-conservatory/blob/master/openssl/)
> on GitHub, along with an excellent ten-page paper "Everything You've Always Wanted to Know About Certificate
> Validation With OpenSSL (but Were Afraid to Ask)." Notably, their example code fails to handle wildcard certs.


## Putting it all together

At the bottom of this post you'll find the complete code for our very simple C++14 HTTPS client.
When you compile and run it, it should fetch the home page of [duckduckgo.com](https://duckduckgo.com)
over an encrypted HTTPS connection.

Suppose you want to try it out with our simple HTTPS server. (Remember, our server pretends to be
`duckduckgo.com`, with a self-signed certificate we issued to ourselves.) We just need to change two lines.
First, we need to make the initial TCP connection to our server, not the real DuckDuckGo:

    -   auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:443"));
    +   auto bio = my::UniquePtr<BIO>(BIO_new_connect("localhost:8080"));

And second, we need to tell the `SSL_CTX` that we trust the server's certificate:

    -   if (SSL_CTX_set_default_verify_paths(ctx.get()) != 1) {
    +   if (SSL_CTX_load_verify_locations(ctx.get(), "server-certificate.pem", nullptr) != 1) {

That's it! If you make those two changes to the client's code, and recompile, then
you'll be able to spin up the HTTPS server from [part 3](/blog/2020/01/26/openssl-part-3/)
and connect to it with this client, as follows:

    $ ./https-server >/dev/null &
    $ ./https-client
    HTTP/1.1 200 OK
    Content-Length: 10

    okay cool
    $

----

Godbolt Compiler Explorer doesn’t support _running_ programs that do networking,
but you can see the code on Godbolt [here](https://godbolt.org/z/__HUeH) anyway.

    // g++ -std=c++14 https-client.cpp $(pkg-config --cflags --libs openssl) -o https-client
    #include <memory>
    #include <stdarg.h>
    #include <stdexcept>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <string>
    #include <vector>

    #include <openssl/bio.h>
    #include <openssl/err.h>
    #include <openssl/ssl.h>
    #include <openssl/x509v3.h>

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

    void send_http_request(BIO *bio, const std::string& line, const std::string& host)
    {
        std::string request = line + "\r\n";
        request += "Host: " + host + "\r\n";
        request += "\r\n";

        BIO_write(bio, request.data(), request.size());
        BIO_flush(bio);
    }

    SSL *get_ssl(BIO *bio)
    {
        SSL *ssl = nullptr;
        BIO_get_ssl(bio, &ssl);
        if (ssl == nullptr) {
            my::print_errors_and_exit("Error in BIO_get_ssl");
        }
        return ssl;
    }

    void verify_the_certificate(SSL *ssl, const std::string& expected_hostname)
    {
        int err = SSL_get_verify_result(ssl);
        if (err != X509_V_OK) {
            const char *message = X509_verify_cert_error_string(err);
            fprintf(stderr, "Certificate verification error: %s (%d)\n", message, err);
            exit(1);
        }
        X509 *cert = SSL_get_peer_certificate(ssl);
        if (cert == nullptr) {
            fprintf(stderr, "No certificate was presented by the server\n");
            exit(1);
        }
    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        if (X509_check_host(cert, expected_hostname.data(), expected_hostname.size(), 0, nullptr) != 1) {
            fprintf(stderr, "Certificate verification error: X509_check_host\n");
            exit(1);
        }
    #else
        // X509_check_host is called automatically during verification,
        // because we set it up in main().
        (void)expected_hostname;
    #endif
    }

    } // namespace my

    int main()
    {
    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        SSL_library_init();
        SSL_load_error_strings();
    #endif

        /* Set up the SSL context */

    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_client_method()));
    #else
        auto ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
    #endif
        if (SSL_CTX_set_default_verify_paths(ctx.get()) != 1) {
            my::print_errors_and_exit("Error setting up trust store");
        }

        auto bio = my::UniquePtr<BIO>(BIO_new_connect("duckduckgo.com:443"));
        if (bio == nullptr) {
            my::print_errors_and_exit("Error in BIO_new_connect");
        }
        if (BIO_do_connect(bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_connect");
        }
        auto ssl_bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(ctx.get(), 1))
            ;
        SSL_set_tlsext_host_name(my::get_ssl(ssl_bio.get()), "duckduckgo.com");
    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_set1_host(my::get_ssl(ssl_bio.get()), "duckduckgo.com");
    #endif
        if (BIO_do_handshake(ssl_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_handshake");
        }
        my::verify_the_certificate(my::get_ssl(ssl_bio.get()), "duckduckgo.com");

        my::send_http_request(ssl_bio.get(), "GET / HTTP/1.1", "duckduckgo.com");
        std::string response = my::receive_http_message(ssl_bio.get());
        printf("%s", response.c_str());
    }

This series continues with ["OpenSSL client and server from scratch, part 5."](/blog/2020/01/28/openssl-part-5/)
