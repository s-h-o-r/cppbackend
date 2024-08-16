#pragma once

#include <boost/json.hpp>

#include "http_server.h"
#include "model.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace fs = std::filesystem;

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace http_handler {

enum class Extention {
    htm, html, css, txt, js, json, xml,
    png, jpg, jpe, jpeg, gif, bmp, ico,
    tiff, tif, svg, svgz, mp3
};

class Uri {
public:
    explicit Uri(std::string_view uri) 
        : uri_(EncodeUri(uri))
        , canonical_uri_(fs::weakly_canonical(uri)) {
    }

    const fs::path& GetRawUri() const;
    const fs::path& GetCanonicalUri() const;

    std::optional<Extention> GetFileExtention() const;

    bool IsSubPath(fs::path base) const;

private:
    fs::path uri_;
    fs::path canonical_uri_;
    std::unordered_map<std::string, Extention> extention_map_ = {
        {".htm", Extention::htm}, {".html", Extention::html}, {".css", Extention::css},
        {".txt", Extention::txt}, {".js", Extention::js}, {".json", Extention::json},
        {".xml", Extention::xml}, {".png", Extention::png}, {".jpg", Extention::jpg},
        {".jpe", Extention::jpe}, {".jpeg", Extention::jpeg}, {".gif", Extention::gif},
        {".bmp", Extention::bmp}, {".ico", Extention::ico}, {".tiff", Extention::tiff},
        {".tif", Extention::tif}, {".svg", Extention::svg}, {".svgz", Extention::svgz},
        {".mp3", Extention::mp3}
    };

    std::string EncodeUri(std::string_view uri) const;
};


class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) const {
        using namespace std::literals;

        std::string_view target = req.target();
        if (target.size() >= 4 && target.substr(0, 5) == "/api/"sv) {
            SendApiResponse(req, std::forward<Send>(send), target);
        } else {
            SendStaticFile(req, std::forward<Send>(send), target);
        }
    }

private:
    model::Game& game_;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view APP_JSON = "application/json";
        constexpr static std::string_view APP_XML = "application/xml";
        constexpr static std::string_view APP_BINARY = "application/octet-stream";

        constexpr static std::string_view TXT_HTML = "text/html";
        constexpr static std::string_view TXT_CSS = "text/css";
        constexpr static std::string_view TXT_PLAIN = "text/plain";
        constexpr static std::string_view TXT_JS = "text/javascript";

        constexpr static std::string_view IMG_PNG = "image/png";
        constexpr static std::string_view IMG_JPEG = "image/jpeg";
        constexpr static std::string_view IMG_GIF = "image/gif";
        constexpr static std::string_view IMG_BMP = "image/bmp";
        constexpr static std::string_view IMG_ICON = "image/vnd.microsoft.icon";
        constexpr static std::string_view IMG_TIFF = "image/tiff";
        constexpr static std::string_view IMG_SVG_XML = "image/svg+xml";

        constexpr static std::string_view AUDIO_MPEG = "audio/mpeg";
    };

    const std::unordered_map<Extention, std::string_view> content_type_map_ = {
        {Extention::htm, ContentType::TXT_HTML}, {Extention::html, ContentType::TXT_HTML}, 
        {Extention::css, ContentType::TXT_CSS},
        {Extention::txt, ContentType::TXT_PLAIN}, 
        {Extention::js, ContentType::TXT_JS},
        {Extention::json, ContentType::APP_JSON},
        {Extention::xml, ContentType::APP_XML}, 
        {Extention::png, ContentType::IMG_PNG},
        {Extention::jpg, ContentType::IMG_JPEG}, {Extention::jpe, ContentType::IMG_JPEG}, {Extention::jpeg, ContentType::IMG_JPEG},
        {Extention::gif, ContentType::IMG_GIF},
        {Extention::bmp, ContentType::IMG_BMP}, 
        {Extention::ico, ContentType::IMG_ICON},
        {Extention::tiff, ContentType::IMG_TIFF}, {Extention::tif, ContentType::IMG_TIFF},
        {Extention::svg, ContentType::IMG_SVG_XML}, {Extention::svgz, ContentType::IMG_SVG_XML},
        {Extention::mp3, ContentType::AUDIO_MPEG}
    };

    template <typename Request, typename Response>
    void FillBasicInfo(Request& req, Response& response) const {
        response.version(req.version());
        response.keep_alive(req.keep_alive());
    }

    template <typename Request, typename Send>
    void SendApiResponse(Request& req, Send&& send, std::string_view target) const {
        http::response<http::string_body> response;
        FillBasicInfo(req, response);

        switch (req.method()) {
            case http::verb::get:
                ProcessApiTarget(response, target);
                break;

            case http::verb::head:
                ProcessApiTarget(response, target);
                response.body().clear();
                break;

            default:
                MakeErrorApiResponse(response, http::status::method_not_allowed);
                break;
        }
        send(response);
    }

    template <typename Request, typename Send>
    void SendStaticFile(Request& req, Send&& send, std::string_view target) const {
        http::response<http::file_body> response;
        FillBasicInfo(req, response);

        switch (req.method()) {
            case http::verb::get:
                ProcessStaticFileTarget(response, target);
                break;

            case http::verb::head:
                ProcessStaticFileTarget(response, target);
                response.body().close();
                break;

            default:
                MakeErrorStaticFileResponse(response, http::status::method_not_allowed);
                break;
        }

        send(response);
    }

    void ProcessApiTarget(http::response<http::string_body>& response, std::string_view target) const;
    void ProcessStaticFileTarget(http::response<http::file_body>& response, std::string_view target) const;

    void MakeErrorApiResponse(http::response<http::string_body>& response, http::status status) const;
    void MakeErrorStaticFileResponse(http::response<http::file_body>& response, http::status status) const;

    std::string ParseMapToJson(const model::Map* map) const;
};

}  // namespace http_handler
