#pragma once

#include <boost/json.hpp>

#include "http_server.h"
#include "model.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace http_handler {

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        // Обработать запрос request и отправить ответ, используя send
        http::response<Body, http::basic_fields<Allocator>> response;
        FillBasicInfo(req, response);

        switch (req.method()) {
            case http::verb::get:
                ProcessGetRequest(req, response);
                break;

            default:
                MakeErrorResponse(response, http::status::method_not_allowed);
                break;
        }

        send(response);
    }

private:
    model::Game& game_;

    struct ContentType {
        ContentType() = delete;
        constexpr static std::string_view APP_JSON = "application/json";
        constexpr static std::string_view TXT_PLAIN = "text/plain";
    };

    template <typename Body, typename Allocator>
    void FillBasicInfo(http::request<Body, http::basic_fields<Allocator>>& req,
                       http::response<Body, http::basic_fields<Allocator>>& response) {
        response.version(req.version());
        response.keep_alive(req.keep_alive());
        response.set(http::field::content_type, ContentType::APP_JSON);
    }

    template <typename Body, typename Allocator>
    void ProcessGetRequest(http::request<Body, http::basic_fields<Allocator>>& req, 
                           http::response<Body, http::basic_fields<Allocator>>& response) {
        using namespace std::literals;
        
        std::string_view target = req.target();
        if (target.substr(0, 12) == "/api/v1/maps"s) {
            ProcessApiTarget(response, target);
        } else if () {
            
        } else {
            MakeErrorResponse(response, http::status::bad_request);
        }
    }

    template <typename Body, typename Allocator>
    void ProcessApiTarget(http::response<Body, http::basic_fields<Allocator>>& response,
                          std::string_view target) {
        if (target.size() > 13) {
            std::string map_name(target.begin() + 13,
                *(target.end() - 1) == '/' ? target.end() - 1 : target.end());

            const model::Map* map = game_.FindMap(model::Map::Id(map_name));
            if (map == nullptr) {
                MakeErrorResponse(response, http::status::not_found);
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
    }



    template <typename Body, typename Allocator>
    void MakeErrorResponse(http::response<Body, http::basic_fields<Allocator>>& response,
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
                response.set(http::field::allow, "GET"sv);
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

    std::string ParseMapToJson(const model::Map* map);
};

}  // namespace http_handler
