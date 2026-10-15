#pragma once
#include <string>
#include <chrono>
#include <format>
#include <utility>

namespace ribomation::weatherdata {
    namespace ch = std::chrono;
    using TimePnt = ch::steady_clock::time_point;
    using std::string;

    class Elapsed {
        string name_;
        TimePnt start;

    public:
        Elapsed() : start{ch::steady_clock::now()} { }

        explicit Elapsed(string n)
            : name_{std::move(n)},
              start{ch::steady_clock::now()} { }

        [[nodiscard]] auto time() const {
            auto stop = ch::steady_clock::now();
            auto interval = ch::duration_cast<ch::milliseconds>(stop - start);
            return interval.count();
        }

        // e.g. "987 ms", "12.3 s", "2 min 8 s", "1 h 2 min 8 s"
        [[nodiscard]] auto to_string() const -> string {
            return pretty(ch::milliseconds{time()});
        }

        [[nodiscard]] static auto pretty(ch::milliseconds ms) -> string {
            using namespace std::chrono_literals;
            if (ms < 1s) return std::format("{} ms", ms.count());
            if (ms < 1min) return std::format("{}.{} s", ms.count() / 1000, ms.count() % 1000 / 100);

            auto hms = ch::hh_mm_ss{ms};
            auto h = hms.hours().count();
            auto m = hms.minutes().count();
            auto s = hms.seconds().count();
            if (ms < 1h) return std::format("{} min {} s", m, s);
            return std::format("{} h {} min {} s", h, m, s);
        }

        [[nodiscard]] auto const& name() const {
            return name_;
        }
    };
}
