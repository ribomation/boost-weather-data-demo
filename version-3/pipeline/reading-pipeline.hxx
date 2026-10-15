#pragma once
#include <concepts>
#include <ios>
#include <functional>

#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>

#include "tar-reader.hxx"
#include "csv-reader.hxx"
#include "observation.hxx"
#include "config.hxx"

namespace ribomation::weatherdata {
    namespace io = boost::iostreams;

    template<typename S>
    using category_t = typename io::category_of<S>::type;

    template<typename S>
    concept StreamSource = requires(S src, char* buffer, std::streamsize n)
    {
        requires std::copy_constructible<S>;
        requires std::same_as<typename io::char_type_of<S>::type, char>;
        requires std::derived_from<category_t<S>, io::input>;
        requires std::derived_from<category_t<S>, io::device_tag>;
        { src.read(buffer, n) } -> std::same_as<std::streamsize>;
    };
    static_assert(StreamSource<http_source>);
    static_assert(StreamSource<io::file_source>);


    template<StreamSource Source>
    void pipeline(Source src, Config const& cfg, std::function<void(Observation&)> handle) {
        constexpr auto BUF_SIZE = static_cast<std::streamsize>(64 * 1024);
        auto pipe = io::filtering_istream{};
        pipe.push(io::gzip_decompressor{});
        pipe.push(std::move(src), BUF_SIZE);
        pipe.exceptions(std::ios::badbit);

        auto tar = TarReader{pipe};
        while (auto entry = tar.next()) {
            if (not entry->is_file()) continue;

            auto csv = CSVReader{tar.content_stream()};
            auto mapper = ObservationMapper{cfg.unit, cfg.precipitation_mm};
            while (csv.next(mapper)) {
                handle(mapper.obs);
            }
        }
    }

}
