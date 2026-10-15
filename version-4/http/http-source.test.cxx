#include <stdexcept>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "http-source.hxx"

using ribomation::weatherdata::http_source;
using Catch::Matchers::ContainsSubstring;

namespace {
    // network hiccups, that has nothing to do with the code under test
    auto is_transient_network_error(std::string const& msg) -> bool {
        for (auto marker : {"Connection reset", "end of stream", "timed out", "Network is unreachable"}) {
            if (msg.contains(marker)) return true;
        }
        return false;
    }

    // returns the error message from http_source(url), or "" if it succeeded.
    // skips the current test section, if the network itself is the problem.
    auto error_of(char const* url) -> std::string {
        auto msg = std::string{};
        try {
            http_source{url};
        } catch (std::exception const& e) {
            msg = e.what();
        }
        if (is_transient_network_error(msg)) {
            SKIP("network problem: " << msg);
        }
        return msg;
    }
}

TEST_CASE("http-source") {
    SECTION("plain http is refused") {
        constexpr auto url = "http://example.com/";
        CHECK_THROWS_WITH(http_source(url), ContainsSubstring("only https is supported"));
    }

    SECTION("404 not found") {
        constexpr auto url = "https://www.ncei.noaa.gov/data/global-summary-of-the-day/access/1901/nope.tar.gz";
        auto error = error_of(url);
        CHECK_THAT(error, ContainsSubstring("HTTP 404"));
    }

    SECTION("unknown host") {
        constexpr auto url = "https://no.such.host.com/data/global-summary-of-the-day/access/1901/nope.tar.gz";
        CHECK_THROWS_WITH(http_source(url), ContainsSubstring("Host not found"));
    }

    SECTION("connection refused") {
        constexpr auto url = "https://localhost:9/x";
        CHECK_THROWS_WITH(http_source(url), ContainsSubstring("Connection refused"));
    }

    SECTION("broken certificates") {
        // badssl.com has servers with deliberately broken certificates
        // error_of() must be called outside of CHECK(), else a SKIP() is reported as a failure
        auto expired     = error_of("https://expired.badssl.com/");
        auto wrong_host  = error_of("https://wrong.host.badssl.com/");
        auto self_signed = error_of("https://self-signed.badssl.com/");
        auto valid       = error_of("https://badssl.com/");     // a valid certificate, as a control
        CHECK_THAT(expired,     ContainsSubstring("certificate verify failed"));
        CHECK_THAT(wrong_host,  ContainsSubstring("certificate verify failed"));
        CHECK_THAT(self_signed, ContainsSubstring("certificate verify failed"));
        CHECK(valid.empty());
    }

}
