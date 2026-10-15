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
#include "byte-counter.hxx"
#include "pipeline-counters.hxx"


namespace ribomation::weatherdata {
    namespace io = boost::iostreams;
    using std::uint64_t;

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
    auto pipeline(Source src, Config const& cfg, std::function_ref<void(Observation&)> handle)
        -> PipelineCounters
    {
        constexpr auto BUF_SIZE = static_cast<std::streamsize>(64 * 1024);
        auto pipe = io::filtering_istream{};
        pipe.push(byte_counter{});              // #0: TAR bytes
        pipe.push(io::gzip_decompressor{});     // #1:
        pipe.push(byte_counter{});              // #2: GZIP bytes
        pipe.push(std::move(src), BUF_SIZE);    // #3:
        pipe.exceptions(std::ios::badbit);

        auto cnt = PipelineCounters{};
        auto tar = TarReader{pipe};
        while (auto entry = tar.next()) {
            if (not entry->is_file()) continue;

            ++cnt.files;
            auto csv = CSVReader{tar.content_stream()};
            auto mapper = ObservationMapper{cfg.unit, cfg.precipitation_mm};
            while (csv.next(mapper)) { handle(mapper.obs); }
        }

        cnt.uncompress_bytes = pipe.component<byte_counter>(0)->count();
        cnt.compress_bytes   = pipe.component<byte_counter>(2)->count();
        return cnt;
    }

}
