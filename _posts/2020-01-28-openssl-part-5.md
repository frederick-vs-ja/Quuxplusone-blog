---
layout: post
title: 'OpenSSL client and server from scratch, part 5'
date: 2020-01-28 00:01:00 +0000
tags:
  library-design
  networking
  web
---

This is a continuation of yesterday's post,
["OpenSSL client and server from scratch, part 4."](/blog/2020/01/27/openssl-part-4/)
For the final blog post in this series, I want to show how to stack SSL BIOs one in front
of the other, so that we have a TLS connection tunneled over another TLS connection.
This "TLS over TLS" pattern is used by a special kind of server called an "HTTPS proxy."


## Quick intro to HTTPS proxies

We'll see in a moment how an HTTPS proxy actually works; but first I want to prove that the
thing we'll be building today isn't crazy — that there exist other servers in the world
that work this way. You can find such servers by googling "https proxy list"; for example,
[this list on sslproxies.org](https://www.sslproxies.org). I went down the list until I
found one that worked. (Open proxies like these are either provided as a public service,
and therefore may be firewalled to work for only certain regions of the world; or provided
as a pay service, and therefore reject client connections lacking proper authorization
headers; or, they may be left open unintentionally, and therefore may get shut down as soon
as someone notices. For any of these reasons, you might have to try a few before finding one
that works.)

The one I found to work (as of this writing) was 119.110.205.66:443, listed as being somewhere
in Thailand. Although the site lists only IP addresses, we can often find a server name by
asking the server for its certificate and looking in the CN and SAN fields (as discussed
in [part 4](/blog/2020/01/27/openssl-part-4/)).

    openssl s_client -connect 119.110.205.66:443 -showcerts

From the certificate and a bit of human guesswork, I established that this HTTPS proxy
is properly known as `https://rtsd.mi.th:443`. ("RTSD" is the
[Royal Thai Survey Department](https://en.wikipedia.org/wiki/Royal_Thai_Survey_Department),
a branch of the Royal Thai Armed Forces.)

Another tool we'll be using is [ipify.org](https://www.ipify.org), which is like one of those
"What's My IP Address" services except without all the malware and banner ads.
To find your machine's IP address, you just run this `curl` command line:

    $ curl 'https://api.ipify.org?format=json'
    {"ip":"148.555.555.555"}

(I've redacted my home IP address, for paranoia's sake.)

So now let's put these two tools together!

    $ curl -p --proxy https://rtsd.mi.th:443 'https://api.ipify.org?format=json'
    {"ip":"119.110.205.66"}

When I tell `curl` to use an HTTPS proxy, it first connects to the proxy, and then — over that connection —
sends a request to fetch `api.ipify.org`. As far as the origin server `api.ipify.org` is concerned,
the request is coming from `rtsd.mi.th` in Thailand, not from my home network. So `api.ipify.org` sends
`{"ip":"119.110.205.66"}` back to `rtsd.mi.th`, and `rtsd.mi.th` forwards that reply back to me.


## Changes to the client

Here's our new client's `main()` routine. It begins much like the old client,
except that I've changed all the hostnames from `duckduckgo.com` to `rtsd.mi.th`:

    int main()
    {
        auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
        if (SSL_CTX_set_default_verify_paths(proxy_ctx.get()) != 1) {
            my::print_errors_and_exit("Error setting up trust store");
        }

        auto bio = my::UniquePtr<BIO>(BIO_new_connect("rtsd.mi.th:443"));
        if (bio == nullptr) {
            my::print_errors_and_exit("Error in BIO_new_connect");
        }
        if (BIO_do_connect(bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_connect");
        }
        auto ssl_bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(proxy_ctx.get(), 1))
            ;
        SSL_set_tlsext_host_name(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");
        SSL_set1_host(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");
        if (BIO_do_handshake(ssl_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_handshake");
        }
        my::verify_the_certificate(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");

        my::send_http_request(ssl_bio.get(), "CONNECT api.ipify.org:443 HTTP/1.1", "rtsd.mi.th");
        std::string response = my::receive_http_message(ssl_bio.get());
        printf("%s\n", response.c_str());

Notice that the HTTP request we send to `rtsd.mi.th` uses the HTTP method `CONNECT`, not `GET`.
We have to give it a hostname and port to connect to; but we don't have to tell it anything about
what path we're ultimately going to `GET`. We expect that its response will be
"200 Connection established."

Now here comes the new part!
We take our TLS connection, `ssl_bio`, and we slap a _second SSL filter BIO_ in front of it.
Now data that we `BIO_write` into `inner_bio` has to pass through not one, but _two_, SSL filter BIOs
before it reaches the TCP socket BIO at the end of the chain.

I'll set up a new `SSL_CTX` for the inner TLS connection, with its own trust store, just to prove
that the outer and inner TLS connections don't need to share anything in common. In particular,
since the inner TLS connection is established with someone who passes certificate verification
for `api.ipify.org`, we can be confident that our request to `GET /?format=json` is unintelligible
to any eavesdroppers — even to the proxy server `rtsd.mi.th` itself (unless the Thai military is
capable of forging certificates for arbitrary domains).

        auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
        if (SSL_CTX_set_default_verify_paths(inner_ctx.get()) != 1) {
            my::print_errors_and_exit("Error setting up trust store");
        }

        auto inner_bio = std::move(ssl_bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(inner_ctx.get(), 1))
            ;
        SSL_set_tlsext_host_name(my::get_ssl(inner_bio.get()), "api.ipify.org");
        SSL_set1_host(my::get_ssl(inner_bio.get()), "api.ipify.org");
        if (BIO_do_handshake(inner_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_handshake");
        }
        my::verify_the_certificate(my::get_ssl(inner_bio.get()), "api.ipify.org");

        my::send_http_request(inner_bio.get(), "GET /?format=json HTTP/1.1", "api.ipify.org");
        response = my::receive_http_message(inner_bio.get());
        printf("%s\n", response.c_str());
    }

If we compile and run this client code ([Godbolt](https://godbolt.org/z/n9QmHx)), we'll see it doing the
same thing that `curl` was doing. The only difference is that `curl` dumps only the message body,
whereas our client also dumps the message headers.

    $ ./https-proxy-client
    HTTP/1.0 200 Connection Established
    Proxy-agent: Apache/2.4.18 (Unix) OpenSSL/1.0.2h PHP/7.0.8 mod_perl/2.0.8-dev Perl/v5.16.3


    HTTP/1.1 200 OK
    Server: Cowboy
    Connection: keep-alive
    Content-Type: application/json
    Vary: Origin
    Date: Mon, 27 Jan 2020 20:17:51 GMT
    Content-Length: 23
    Via: 1.1 vegur

    {"ip":"119.110.205.66"}


## Printing out an X.509 certificate

I wondered if I was being too naïve when I said that `rtsd.mi.th` wasn't forging certificates to eavesdrop
on us. After all, this HTTPS proxy _is_ apparently being run by the armed forces of a nation-state...
So let's have our client print out the certificate it receives over the inner TLS connection. If that certificate
doesn't match the one we fetched straight from `api.ipify.org` itself, then we'll consider something fishy.

Here's how to dump a certificate in PEM format:

    std::string to_pem(X509 *cert) {
        StringBIO bio;
        PEM_write_bio_X509(bio.bio(), cert);
        return std::move(bio).str();
    }

To dump the entire chain of certificates presented by the server, you'd call `to_pem` on each element
of [`SSL_get_peer_cert_chain`](https://www.openssl.org/docs/man1.1.1/man3/SSL_get_peer_cert_chain.html);
that's out of scope for this blog post.

I've verified that the certificate presented by the-thing-we-hope-is-`api.ipify.org` via the proxy
is identical to the certificate presented by `api.ipify.org` when we connect directly.


## Changes to the server

A real HTTPS proxy would have to act as both a server and a client — a server from `curl`'s
point of view, and a client from `api.ipify.org`'s point of view. Its main loop would look
something like this:

    while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
        bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(proxy_ctx.get(), 0))
            ;
        try {
            std::string request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());

            std::string to_whom = my::get_request_path(request);
            auto upstream_bio = my::UniquePtr<BIO>(BIO_new_connect(to_whom.c_str()));
            if (upstream_bio == nullptr || BIO_do_connect(upstream_bio.get()) <= 0) {
                my::send_http_error_response(bio.get(), 500);
                throw std::runtime_error("error connecting to upstream server");
            }
            my::send_http_connection_established_response(bio.get());
            std::thread(
                my::shuttle_bytes_back_and_forth,
                std::move(bio), std::move(upstream_bio)
            ).detach();
        } catch (const std::exception& ex) {
            printf("Worker exited with exception:\n%s\n", ex.what());
        }
    }

The implementation of `my::shuttle_bytes_back_and_forth` is out of scope for this blog post.
(It would have to do non-blocking reads on both BIOs, taking anything it managed to read and
writing it into the other BIO. Essentially this creates a "TCP tunnel" between the client and
the origin server, except that the client side of the tunnel is layered within
a TLS connection.)

For the purposes of this blog post, let's just fake it. Our old HTTPS server got away with
pretending to be `duckduckgo.com`; let's have our new one pretend to be both
`rtsd.mi.th` and `api.ipify.org`!  We just set up two new `SSL_CTX`s:

    auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
    SSL_CTX_set_min_proto_version(proxy_ctx.get(), TLS1_2_VERSION);
    SSL_CTX_use_certificate_file(proxy_ctx.get(), "proxy-certificate.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(proxy_ctx.get(), "proxy-private-key.pem", SSL_FILETYPE_PEM);

    auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
    SSL_CTX_set_min_proto_version(inner_ctx.get(), TLS1_2_VERSION);
    SSL_CTX_use_certificate_file(inner_ctx.get(), "inner-certificate.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(inner_ctx.get(), "inner-private-key.pem", SSL_FILETYPE_PEM);

And then our main loop goes like this:

    while (auto bio = my::accept_new_tcp_connection(accept_bio.get())) {
        bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(proxy_ctx.get(), 0))
            ;
        try {
            std::string request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());
            std::string response = "HTTP/1.1 200 Connection established\r\n\r\n";
            BIO_write(bio.get(), response.data(), response.size());

            bio = std::move(bio)
                | my::UniquePtr<BIO>(BIO_new_ssl(inner_ctx.get(), 0))
                ;
            request = my::receive_http_message(bio.get());
            printf("Got request:\n");
            printf("%s\n", request.c_str());
            my::send_http_response(bio.get(), "okay cool\n");
        } catch (const std::exception& ex) {
            printf("Worker exited with exception:\n%s\n", ex.what());
        }
    }

We can test this code with `curl -p`, just like we tested our HTTPS server in
[part 3](/blog/2020/01/26/openssl-part-3/). First we generate the keypairs
and certificates that the server expects to use:

    openssl ecparam -genkey -name prime256v1 -noout -out proxy-private-key.pem
    openssl req -new -x509 -sha256 -key proxy-private-key.pem -subj "/CN=rtsd.mi.th" -out proxy-certificate.pem

    openssl ecparam -genkey -name prime256v1 -noout -out inner-private-key.pem
    openssl req -new -x509 -sha256 -key inner-private-key.pem -subj "/CN=*.ipify.org" -out inner-certificate.pem

Then spin up the server, and run the same `curl` as before (with some additional
options, as explained in [part 3](/blog/2020/01/26/openssl-part-3/)).

    $ curl -p --proxy-cacert proxy-certificate.pem \
           --resolve rtsd.mi.th:8080:127.0.0.1 \
           --cacert inner-certificate.pem \
           --proxy https://rtsd.mi.th:8080/ \
           'https://api.ipify.org/?format=json'
    okay cool
    $

Hooray!


## Complete code for the client

You can see the client code on Godbolt [here](https://godbolt.org/z/hLz5g7).

    // g++ -std=c++14 https-proxy-client.cpp $(pkg-config --cflags --libs openssl) -o https-proxy-client
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

    #if OPENSSL_VERSION_NUMBER < 0x10100000L
        auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_client_method()));
        auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_client_method()));
    #else
        auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
        auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_client_method()));
        SSL_CTX_set_min_proto_version(proxy_ctx.get(), TLS1_2_VERSION);
        SSL_CTX_set_min_proto_version(inner_ctx.get(), TLS1_2_VERSION);
    #endif
        if (SSL_CTX_set_default_verify_paths(proxy_ctx.get()) != 1) {
            my::print_errors_and_exit("Error setting up trust store");
        }
        if (SSL_CTX_set_default_verify_paths(inner_ctx.get()) != 1) {
            my::print_errors_and_exit("Error setting up inner trust store");
        }

        auto bio = my::UniquePtr<BIO>(BIO_new_connect("rtsd.mi.th:443"));
        if (bio == nullptr) {
            my::print_errors_and_exit("Error in BIO_new_connect");
        }
        if (BIO_do_connect(bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_connect");
        }
        auto ssl_bio = std::move(bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(proxy_ctx.get(), 1))
            ;
        SSL_set_tlsext_host_name(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");
    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_set1_host(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");
    #endif
        if (BIO_do_handshake(ssl_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in BIO_do_handshake");
        }
        my::verify_the_certificate(my::get_ssl(ssl_bio.get()), "rtsd.mi.th");

        my::send_http_request(ssl_bio.get(), "CONNECT api.ipify.org:443 HTTP/1.1", "rtsd.mi.th");
        std::string response = my::receive_http_message(ssl_bio.get());
        printf("%s\n", response.c_str());

        auto inner_bio = std::move(ssl_bio)
            | my::UniquePtr<BIO>(BIO_new_ssl(inner_ctx.get(), 1))
            ;
        SSL_set_tlsext_host_name(my::get_ssl(inner_bio.get()), "api.ipify.org");
    #if OPENSSL_VERSION_NUMBER >= 0x10100000L
        SSL_set1_host(my::get_ssl(inner_bio.get()), "api.ipify.org");
    #endif
        if (BIO_do_handshake(inner_bio.get()) <= 0) {
            my::print_errors_and_exit("Error in inner BIO_do_handshake");
        }
        my::verify_the_certificate(my::get_ssl(inner_bio.get()), "api.ipify.org");

        my::send_http_request(inner_bio.get(), "GET /?format=json HTTP/1.1", "api.ipify.org");
        response = my::receive_http_message(inner_bio.get());
        printf("%s\n", response.c_str());
    }


## Complete code for the server

You can see the server code on Godbolt [here](https://godbolt.org/z/xC46Bk).

    // g++ -std=c++14 https-proxy-server.cpp $(pkg-config --cflags --libs openssl) -o https-proxy-server
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
        auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_method()));
        auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(SSLv23_method()));
    #else
        auto proxy_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
        auto inner_ctx = my::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
        SSL_CTX_set_min_proto_version(proxy_ctx.get(), TLS1_2_VERSION);
        SSL_CTX_set_min_proto_version(inner_ctx.get(), TLS1_2_VERSION);
    #endif

        if (SSL_CTX_use_certificate_file(proxy_ctx.get(), "proxy-certificate.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading proxy certificate");
        }
        if (SSL_CTX_use_PrivateKey_file(proxy_ctx.get(), "proxy-private-key.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading proxy private key");
        }
        if (SSL_CTX_use_certificate_file(inner_ctx.get(), "inner-certificate.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading inner certificate");
        }
        if (SSL_CTX_use_PrivateKey_file(inner_ctx.get(), "inner-private-key.pem", SSL_FILETYPE_PEM) <= 0) {
            my::print_errors_and_exit("Error loading inner private key");
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
                | my::UniquePtr<BIO>(BIO_new_ssl(proxy_ctx.get(), 0))
                ;
            try {
                std::string request = my::receive_http_message(bio.get());
                printf("Got request:\n");
                printf("%s\n", request.c_str());
                std::string response = "HTTP/1.1 200 Connection established\r\n\r\n";
                BIO_write(bio.get(), response.data(), response.size());

                bio = std::move(bio)
                    | my::UniquePtr<BIO>(BIO_new_ssl(inner_ctx.get(), 0))
                    ;
                request = my::receive_http_message(bio.get());
                printf("Got request:\n");
                printf("%s\n", request.c_str());
                my::send_http_response(bio.get(), "okay cool\n");
            } catch (const std::exception& ex) {
                printf("Worker exited with exception:\n%s\n", ex.what());
            }
        }
        printf("\nClean exit!\n");
    }

This concludes my five-day, five-part blog series on OpenSSL clients and servers.
I hope you enjoyed it!

To start again at the beginning, go back to [part 1](/blog/2020/01/24/openssl-part-1/).
