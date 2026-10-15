#pragma once
#include <cstdint>
#include <filesystem>
#include <ios>
#include <memory>
#include <optional>
#include <boost/iostreams/concepts.hpp>
#include <boost/url/url.hpp>


namespace ribomation::weatherdata {
    namespace fs = std::filesystem;
    namespace urls = boost::urls;
    namespace io = boost::iostreams;
    using std::string;
    using std::shared_ptr;

    class http_source : public io::source {
        struct Connection; //forward decl
        shared_ptr<Connection> conn;

    public:
        explicit http_source(urls::url_view url);

        auto read(char* buffer, std::streamsize n) -> std::streamsize;
        auto content_length() const -> std::optional<std::uint64_t>;

        static auto archive_url(string const& base_url, unsigned year) -> urls::url;
        static auto archive_file(fs::path const& cache_dir, unsigned year) -> fs::path;
        static auto download(urls::url_view url, fs::path const& target) -> std::uintmax_t;
    };
}
