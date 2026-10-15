#include <chrono>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include "elapsed.hxx"

using ribomation::weatherdata::Elapsed;
using namespace std::chrono_literals;

TEST_CASE("elapsed") {
    SECTION("milliseconds") {
        CHECK(Elapsed::pretty(0ms) == "0 ms");
        CHECK(Elapsed::pretty(987ms) == "987 ms");
    }

    SECTION("seconds, with one decimal") {
        CHECK(Elapsed::pretty(1000ms) == "1.0 s");
        CHECK(Elapsed::pretty(12345ms) == "12.3 s");
        CHECK(Elapsed::pretty(59999ms) == "59.9 s");    // truncated, not rounded up to 60.0
    }

    SECTION("minutes") {
        CHECK(Elapsed::pretty(1min) == "1 min 0 s");
        CHECK(Elapsed::pretty(128687ms) == "2 min 8 s");
        CHECK(Elapsed::pretty(59min + 59s + 999ms) == "59 min 59 s");
    }

    SECTION("hours") {
        CHECK(Elapsed::pretty(1h) == "1 h 0 min 0 s");
        CHECK(Elapsed::pretty(1h + 2min + 8s) == "1 h 2 min 8 s");
        CHECK(Elapsed::pretty(26h + 5s) == "26 h 0 min 5 s");
    }

    SECTION("to_string of a fresh timer") {
        auto e = Elapsed{};
        CHECK(e.to_string().ends_with(" ms"));
    }
}
