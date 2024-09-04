#pragma once

#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include "http_server.h"
#include "model.h"
#include "player.h"

#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace http_handler {

namespace fs = std::filesystem;
namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

enum class Extention {
    empty, unkown,
    htm, html, css, txt, js, json, xml,
    png, jpg, jpe, jpeg, gif, bmp, ico,
    tiff, tif, svg, svgz, mp3
};

class Uri {
public:
    explicit Uri(std::string_view uri, const fs::path& base);

    const fs::path& GetCanonicalUri() const;

    Extention GetFileExtention() const;

    bool IsSubPath(const fs::path& base) const;

private:
    fs::path uri_;
    fs::path canonical_uri_;

    std::string EncodeUri(std::string_view uri) const;
};

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

template <typename Request, typename Response>
void FillBasicInfo(Request& req, Response& response) {
    response.version(req.version());
    response.keep_alive(req.keep_alive());
}

std::string_view GetMimeType(Extention extention);
std::string ParseMapToJson(const model::Map* map);

using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;

class ApiRequestHandler : std::enable_shared_from_this<ApiRequestHandler> {
public:
    using Strand = net::strand<net::io_context::executor_type>;

    explicit ApiRequestHandler(model::Game& game, net::io_context& ioc)
    : game_(game)
    , ioc_(ioc)
    , strand_{net::make_strand(ioc_)} {
    }

    ApiRequestHandler(const ApiRequestHandler&) = delete;
    ApiRequestHandler& operator=(const ApiRequestHandler&) = delete;

    template <typename Request, typename Send>
    void operator() (Request&& req, Send&& send) {
        std::string_view target = req.target();
        SendApiResponse(req, std::forward<Send>(send), target);
    }

private:
    model::Game& game_;
    net::io_context& ioc_;
    Strand strand_;

    user::Players players_;
    user::PlayerTokens tokens_;


    template <typename Request, typename Send>
    void SendApiResponse(Request& req, Send&& send, std::string_view target) {
        using namespace std::literals;

        StringResponse response;
        try {
            FillBasicInfo(req, response);

            switch (req.method()) {
                case http::verb::get:
                case http::verb::head:
                    if (target.substr(0, 12) == "/api/v1/maps"sv) {
                        ProcessApiMaps(response, target);
                    } else if (target.substr(0, 20) == "/api/v1/game/players"sv) {
                        ProcessApiPlayers(req, response);
                    } else {
                        if (target.substr(0, 17) == "/api/v1/game/join"sv) {
                            MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_join,
                                                 "Only POST method is expected"sv);
                            break;
                        }
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request, "Bad request"sv);
                    }
                    break;

                case http::verb::post:
                    if (target.substr(0, 17) == "/api/v1/game/join"sv) {
                        ProcessApiJoin(req, response);
                    } else {
                        MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::bad_request, "Bad request"sv);
                    }
                    break;
                default:
                    MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::invalid_method_common, "Invalid method"sv);
                    break;
            }
        } catch (...) {
            MakeErrorApiResponse(response, ApiRequestHandler::ErrorCode::unknown, "Unknown error"sv);
        }
        send(response);
    }

    void ProcessApiMaps(StringResponse& response, std::string_view target) const;

    template <typename Request>
    void ProcessApiPlayers(Request& request, StringResponse& response) const {
        using namespace std::literals;

        response.set(http::field::cache_control, "no-cache");

        if (!request.base().count("Authorization")) {
            MakeErrorApiResponse(response, ErrorCode::invalid_token, "Authorization header is missing"sv);
            return;
        }

        std::string_view token_prefix = request.base().at("Authorization").substr(0, 7);
        std::string_view token = request.base().at("Authorization").substr(7); // Убираем Bearer перед токеном

        if (std::size_t token_size = 32; token_prefix != "Bearer " || token.size() != token_size) {
            MakeErrorApiResponse(response, ErrorCode::invalid_token, "Authorization header is missing"sv);
            return;
        }

        const user::Player* player = tokens_.FindPlayerByToken(user::Token{std::string(token)});
        if (player == nullptr) {
            MakeErrorApiResponse(response, ErrorCode::unknown_token, "Player token has not been found"sv);
            return;
        }

        const auto& dogs = player->GetGameSession()->GetDogs();
        json::object players_on_map_json;
        for (const auto& [id, dog] : dogs) {
            players_on_map_json[std::to_string(*id)] = {{"name", dog->GetName()}};
        }

        response.body() = json::serialize(json::value(std::move(players_on_map_json)));

        response.set(http::field::content_type, ContentType::APP_JSON);
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    template <typename Request>
    void ProcessApiJoin(Request& request, StringResponse& response) {
        using namespace std::literals;

        response.set(http::field::cache_control, "no-cache");

        boost::system::error_code ec;
        json::value request_body = json::parse(request.body(), ec);
        if (ec || !(request_body.if_object() && request_body.as_object().count("userName") && request_body.as_object().count("mapId"))) {
            MakeErrorApiResponse(response, ErrorCode::invalid_argument, "Join game request parse error"sv);
            return;
        }

        std::string user_name = std::string(request_body.as_object().at("userName").as_string());
        std::string map_id = std::string(request_body.as_object().at("mapId").as_string());

        if (user_name.empty()) {
            MakeErrorApiResponse(response, ErrorCode::invalid_argument, "Invalid name"sv);
            return;
        }

        if (!game_.FindMap(model::Map::Id{map_id})) {
            MakeErrorApiResponse(response, ErrorCode::map_not_found, "Map not found"sv);
            return;
        }

        model::GameSession* session = game_.GetGameSession(model::Map::Id{map_id});
        if (session == nullptr) {
            session = &game_.StartGameSession(ioc_, game_.FindMap(model::Map::Id{map_id}));
        }

        model::Dog* dog = session->AddDog(user_name);

        user::Token player_token = tokens_.AddPlayer(&players_.Add(dog, session));

        json::value jv = {
            {"authToken", *player_token},
            {"playerId", *dog->GetId()}
        };

        response.body() = json::serialize(jv);

        response.set(http::field::content_type, ContentType::APP_JSON);
        response.content_length(response.body().size());
        response.result(http::status::ok);
    }

    enum class ErrorCode {
        unknown,
        map_not_found, invalid_method_common, invalid_method_join,
        invalid_argument, bad_request, invalid_token, unknown_token
    };

    void MakeErrorApiResponse(StringResponse& response, ApiRequestHandler::ErrorCode code,
                              std::string_view message) const;

    //void MakeErrorApiGetResponse(StringResponse& response, http::status status) const;
    //void MakeErrorApiAuthResponse(StringResponse& response, http::status status, std::string_view message) const;

};

class StaticRequestHandler {
public:
    explicit StaticRequestHandler(std::filesystem::path&& static_files_path)
    : static_files_path_(std::move(static_files_path)) {
    }

    StaticRequestHandler(const StaticRequestHandler&) = delete;
    StaticRequestHandler& operator=(const StaticRequestHandler&) = delete;

    template <typename Request, typename Send>
    void operator() (Request&& req, Send&& send) {
        try {
            std::string_view target = req.target();
            SendStaticFile(req, std::forward<Send>(send), target);
        } catch (...) {
            send(MakeErrorStaticFileResponse(http::status::unknown));
        }
    }

private:
    std::filesystem::path static_files_path_;

    template <typename Request, typename Send>
    void SendStaticFile(Request& req, Send&& send, std::string_view target) const {
        FileResponse response;
        FillBasicInfo(req, response);

        http::status status{};

        switch (req.method()) {
            case http::verb::get:
            case http::verb::head:
                status = ProcessStaticFileTarget(response, target);
                break;

            default:
                auto error_response = MakeErrorStaticFileResponse(http::status::method_not_allowed);
                FillBasicInfo(req, error_response);
                send(error_response);
                return;
        }

        if (status != http::status::ok) {
            auto error_response = MakeErrorStaticFileResponse(status);
            FillBasicInfo(req, error_response);
            send(error_response);
        } else {
            send(response);
        }
    }

    http::status ProcessStaticFileTarget(FileResponse& response, std::string_view target) const;
    StringResponse MakeErrorStaticFileResponse(http::status status) const;
};

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, net::io_context& ioc, std::filesystem::path&& static_files_path)
        : api_handler_(game, ioc)
        , static_handler_(std::move(fs::canonical(static_files_path))) {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        using namespace std::literals;

        std::string_view target = req.target();
        if (target.size() >= 4 && target.substr(0, 5) == "/api/"sv) {
            api_handler_(std::forward<decltype(req)>(req), std::forward<Send>(send));
        } else {
            static_handler_(std::forward<decltype(req)>(req), std::forward<Send>(send));
        }
    }

private:
    ApiRequestHandler api_handler_;
    StaticRequestHandler static_handler_;
};

}  // namespace http_handler
