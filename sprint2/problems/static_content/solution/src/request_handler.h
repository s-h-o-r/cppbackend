#pragma once

#include <boost/json.hpp>

#include "http_server.h"
#include "model.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace http_handler {

class Uri {
public:
    explicit Uri(std::string_view uri) 
        : uri_(EncodeUri(uri))
        , canonical_uri_(fs::weakly_canonical(uri)) {
    }

    const fs::path& GetRawUri() const;
    const fs::path& GetCanonicalUri() const;

    std::string GetFileExtention() const;

    bool IsSubPath(fs::path base);

private:
    fs::path uri_;
    fs::path canonical_uri_;

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
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using namespace std::literals;

        // Обработать запрос request и отправить ответ, используя send
        http::response<Body, http::basic_fields<Allocator>> response;
        FillBasicInfo(req, response);

        std::string_view target = req.target();
        if (target.size() >= 4 && target.substr(0, 5) == "/api/"sv) {
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
        } else {
            switch (req.method()) {
                case http::verb::get:
                    ProcessStaticFileTarget(response, target);
                    break;

                case http::verb::head:
                    ProcessStaticFileTarget(response, target);
                    response.body().clear();
                    break;

                default:
                    MakeErrorStaticFileResponse(response, http::status::method_not_allowed);
                    break;
            }
        }



        send(response);
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

    template <typename Body, typename Allocator>
    void FillBasicInfo(http::request<Body, http::basic_fields<Allocator>>& req,
                       http::response<Body, http::basic_fields<Allocator>>& response) {
        response.version(req.version());
        response.keep_alive(req.keep_alive());
    }

    template <typename Body, typename Allocator>
    void ProcessApiTarget(http::response<Body, http::basic_fields<Allocator>>& response,
                          std::string_view target) {
        using namespace std::literals;

        if (target.substr(0, 12) != "/api/v1/maps"sv) {
            MakeErrorApiResponse(response, http::status::bad_request);
        }

        if (target.size() > 13) {
            std::string map_name(target.begin() + 13,
                *(target.end() - 1) == '/' ? target.end() - 1 : target.end());

            const model::Map* map = game_.FindMap(model::Map::Id(map_name));
            if (map == nullptr) {
                MakeErrorApiResponse(response, http::status::not_found);
                return;
            }

            response.body() = ParseMapToJson(map);
        } else {
            json::array maps_json;
            const model::Game::Maps& maps = game_.GetMaps();
            for (const auto& map : maps) {
                maps_json.push_back({
                    {"id", *map.GetId()}, {"name", map.GetName()}
                                    });
            }
            response.body() = json::serialize(json::value(std::move(maps_json)));
        }
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    template <typename Body, typename Allocator>
    void ProcessStaticFileTarget(http::response<Body, http::basic_fields<Allocator>>& response,
                                 std::string_view target) {
        Uri uri(target);
        if (!uri.IsSubPath(fs::current_path())) {
            MakeErrorStaticFileResponse(response, http::status::bad_request);
            return;
        }
// /// /// /// // // / //// // / 

    }

    template <typename Body, typename Allocator>
    void MakeErrorApiResponse(http::response<Body, http::basic_fields<Allocator>>& response,
                              http::status status) {
        using namespace std::literals;

        response.result(status);
        switch (status) {
            case http::status::not_found:
            {
                json::value jv = {
                    {"code", "mapNotFound"},
                    {"message", "Map not found"}
                };
                response.body() = json::serialize(jv);
                break;
            }

            case http::status::method_not_allowed:
            {
                json::value jv = {
                    {"code", "InvalidMethod"},
                    {"message", "Invalid method"}
                };
                response.body() = json::serialize(jv);
                response.set(http::field::allow, "GET, HEAD"sv);
                break;
            }

            default:
            {
                json::value jv = {
                    {"code", "badRequest"},
                    {"message", "Bad request"}
                };
                response.body() = json::serialize(jv);
                break;
            }
        }
        response.content_length(response.body().size());
    }

    template <typename Body, typename Allocator>
    void MakeErrorStaticFileResponse(http::response<Body, http::basic_fields<Allocator>>& response,
                                     http::status status) {
        using namespace std::literals;
    
        response.result(status);
        response.set(http::field::content_type, ContentType::TXT_PLAIN);
        switch (status) {
            case http::status::not_found:
                response.body() = "File is not found"sv;
                break;

            case http::status::bad_request:
                response.body() = "Target is out of home directory"sv;
                break;
            
            case http::status::method_not_allowed:
                response.body() = "Method not allowed"sv;
                response.set(http::field::allow, "GET, HEAD"sv);
                break;

            default:
                response.body() = "Unknown error"sv;
                break;
        }
        response.content_length(response.body().size());
    }

    std::string ParseMapToJson(const model::Map* map);
};

}  // namespace http_handler
