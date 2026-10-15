#include <chrono>
#include <format>
#include <stdexcept>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/url/parse.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "http-source.hxx"


namespace ribomation::weatherdata {
    namespace beast = boost::beast;
    namespace http = beast::http;
    namespace net = boost::asio;
    namespace ssl = net::ssl;
    namespace io = boost::iostreams;
    using namespace std::chrono_literals;
    using tcp = net::ip::tcp;
    using std::string;

    constexpr auto timeout = 30s;

    struct http_source::Connection {
        net::io_context ioc;
        ssl::context tls{ssl::context::tls_client};
        beast::ssl_stream<beast::tcp_stream> stream{ioc, tls};
        beast::flat_buffer buffer; // what was read ahead of the body
        http::response_parser<http::buffer_body> parser;
        string url; // for error messages

        explicit Connection(urls::url_view u) : url(u.buffer()) {
            if (u.scheme_id() != urls::scheme::https)
                throw std::invalid_argument{"only https is supported: " + url};

            auto host = u.host();
            auto port = u.has_port() ? string(u.port()) : string("443");
            auto target = u.encoded_target().empty() ? string("/") : string(u.encoded_target());

            // Verify the server certificate against the system's CAs, and send the host name (SNI)
            tls.set_default_verify_paths();
            stream.set_verify_mode(ssl::verify_peer);
            stream.set_verify_callback(ssl::host_name_verification(host));
            if (not SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
                throw beast::system_error{
                    beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category())
                };
            }

            auto& socket = beast::get_lowest_layer(stream);
            socket.expires_after(timeout);
            socket.connect(tcp::resolver{ioc}.resolve(host, port));
            stream.handshake(ssl::stream_base::client);

            auto request = http::request<http::empty_body>{http::verb::get, target, 11};
            request.set(http::field::host, host);
            request.set(http::field::user_agent, "weather-data/" APP_VERSION);
            http::write(stream, request);

            parser.body_limit(boost::none); // the default limit is 8 MB, the archive files are larger
            http::read_header(stream, buffer, parser);
            if (auto status = parser.get().result_int(); status != 200) {
                throw std::runtime_error{
                    std::format("HTTP {} {} for {}", status, string(parser.get().reason()), url)
                };
            }
        }
    };


    http_source::http_source(urls::url_view url) {
        try {
            conn = std::make_shared<Connection>(url);
        } catch (boost::system::system_error const& x) {
            throw std::runtime_error{string(url.buffer()) + ": " + x.code().message()};
        }
    }

    auto http_source::read(char* buffer, std::streamsize n) -> std::streamsize {
        auto& c = *conn;
        while (not c.parser.is_done()) {
            // The parser writes straight into the caller's buffer; body().size is what is left of it
            auto& body = c.parser.get().body();
            body.data = buffer;
            body.size = static_cast<std::size_t>(n);

            beast::get_lowest_layer(c.stream).expires_after(timeout);
            auto ec = beast::error_code{};
            http::read(c.stream, c.buffer, c.parser, ec);
            if (ec and ec != http::error::need_buffer) {
                // need_buffer = the buffer is full, not  an error
                throw std::runtime_error{c.url + ": " + ec.message()};
            }
            if (auto received = n - static_cast<std::streamsize>(body.size); received > 0) {
                return received;
            }
        }
        return -1;
    }

    auto http_source::content_length() const -> std::optional<std::uint64_t> {
        if (auto len = conn->parser.content_length()) return *len;
        return std::nullopt;
    }

    auto http_source::archive_url(string const& base_url, unsigned year) -> urls::url {
        auto u = urls::url{urls::parse_uri(base_url).value()}; // throws if malformed

        auto s = u.segments(); //sequence of path segments
        if (not s.empty() and s.back().empty()) {
            s.pop_back(); //drop last, if empty
        }
        s.push_back(std::format("{}.tar.gz", year));

        return u;
    }

    auto http_source::archive_file(fs::path const& cache_dir, unsigned year) -> fs::path {
        return cache_dir / std::format("{}.tar.gz", year);
    }

    auto http_source::download(urls::url_view url, fs::path const& target) -> std::uintmax_t {
        if (target.has_parent_path()) {
            fs::create_directories(target.parent_path());
        }

        auto partial = fs::path{target} += ".part";
        try {
            auto sink = io::file_sink{partial.string(), std::ios::binary};
            if (not sink.is_open()) {
                throw std::runtime_error{"cannot write " + partial.string()};
            }
            io::copy(http_source{url}, sink); //slurp
        } catch (...) {
            fs::remove(partial);
            throw;
        }
        fs::rename(partial, target);
        return fs::file_size(target);
    }

}
