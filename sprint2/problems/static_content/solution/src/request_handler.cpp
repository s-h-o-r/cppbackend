#include "request_handler.h"

namespace model {

void tag_invoke(json::value_from_tag, json::value& jv, const Building& building) {
    auto bounds = building.GetBounds();
    jv = {
        {"x", bounds.position.x}, {"y", bounds.position.y},
        {"w", bounds.size.width}, {"h", bounds.size.height}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const Office& office) {
    auto position = office.GetPosition();
    auto offset = office.GetOffset();
    jv = {
        {"id", *office.GetId()}, {"x", position.x}, {"y", position.y},
        {"offsetX", offset.dx}, {"offsetY", offset.dy}
    };
}

void tag_invoke(json::value_from_tag, json::value& jv, const model::Road& road) {
    auto start = road.GetStart();
    auto end = road.GetEnd();
    if (road.IsVertical()) {
        jv = {
            {"x0", start.x}, {"y0", start.y}, {"y1", end.y}
        };
    } else {
        jv = {
            {"x0", start.x}, {"y0", start.y}, {"x1", end.x}
        };
    }
}


void tag_invoke(json::value_from_tag, json::value& jv, const model::Map& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", json::value_from(map.GetRoads())},
        {"buildings", json::value_from(map.GetBuildings())},
        {"offices", json::value_from(map.GetOffices())}
    };
}
} // namespace model

namespace http_handler {

using namespace std::literals;

namespace details {

std::string EncodeUriSpaces(std::string_view sub_uri) {
    std::string encoded_sub_uri;

    size_t pos = 0;
    while (pos != std::string_view::npos) {
        size_t space_pos = sub_uri.find_first_of('+', pos);
        if (space_pos != std::string_view::npos) {
            encoded_sub_uri += std::string{sub_uri.substr(pos, space_pos - pos)};
            encoded_sub_uri += ' ';
            pos = space_pos + 1;
        } else {
            encoded_sub_uri += std::string{sub_uri.substr(pos)};
            pos = space_pos;
        }
    }
    return encoded_sub_uri;
}

}; // namespace http_handler::details

const fs::path& Uri::GetRawUri() const {
    return uri_;
}

const fs::path& Uri::GetCanonicalUri() const {
    return canonical_uri_;
}

std::optional<Extention> Uri::GetFileExtention() const {
    auto extention = canonical_uri_.extension().string();
    if (extention.empty()) {
        return std::nullopt;
    }

    for (int i = 0; i < extention.length(); ++i) {
        if (std::isupper(extention[i])) {
            extention[i] = std::tolower(extention[i]);
        }
    }

    if (!extention_map_.contains(extention)) {
        return std::nullopt;
    }

    return extention_map_.at(extention);
}

bool Uri::IsSubPath(const fs::path& base) const {
    fs::path path = base / canonical_uri_;
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == canonical_uri_.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string Uri::EncodeUri(std::string_view uri) const {
    std::string encoded_uri;
    
    size_t pos = 0;
    while (pos != std::string_view::npos) {
        size_t encoding_pos = uri.find_first_of('%', pos);

        if (encoding_pos != std::string_view::npos) {
            encoded_uri += details::EncodeUriSpaces(uri.substr(pos, encoding_pos - pos));

            char coded_symbol = std::stoi(std::string{uri.substr(encoding_pos + 1, 2)}, 0, 16);
            encoded_uri += coded_symbol;

            pos = encoding_pos + 3;
        } else {
            encoded_uri += details::EncodeUriSpaces(uri.substr(pos));
            pos = encoding_pos;
        }
    }

    return encoded_uri;
}

void RequestHandler::ProcessApiTarget(http::response<http::string_body>& response,
                                      std::string_view target) const {
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

    response.set(http::field::content_type, ContentType::APP_JSON);
    response.content_length(response.body().size());
    response.result(http::status::ok);
}

http::status RequestHandler::ProcessStaticFileTarget(http::response<http::file_body>& response,
                                                     std::string_view target) const {
    Uri uri(target);
    if (!uri.IsSubPath(static_files_path_)) {
        return http::status::bad_request;
    }

    http::file_body::value_type file;

    if (http_server::sys::error_code ec; file.open(target.data(), beast::file_mode::read, ec), ec) {
        return http::status::not_found;
    }

    auto file_extention = uri.GetFileExtention();

    if (!file_extention.has_value()) {
        response.set(http::field::content_type, ContentType::APP_BINARY);
    } else {
        response.set(http::field::content_type, content_type_map_.at(*file_extention));
    }

    response.body() = std::move(file);
    response.prepare_payload();
    response.result(http::status::ok);

    return http::status::ok;
}

void RequestHandler::MakeErrorApiResponse(http::response<http::string_body>& response,
                                          http::status status) const {
    response.result(status);
    response.set(http::field::content_type, ContentType::APP_JSON);
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

http::response<http::string_body> RequestHandler::MakeErrorStaticFileResponse(http::status status) const {
    http::response<http::string_body> response;
    response.result(status);
    response.set(http::field::content_type, ContentType::TXT_PLAIN);

    std::string_view error_message;
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
    return response;
}

std::string RequestHandler::ParseMapToJson(const model::Map* map) const {

    return json::serialize(json::value_from(*map));
}

}  // namespace http_handler
