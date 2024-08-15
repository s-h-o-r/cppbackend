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
            std::tolower(extention[i]);
        }
    }

    if (!extention_map_.contains(extention)) {
        return std::nullopt;
    }

    return extention_map_.at(extention);
}

bool Uri::IsSubPath(fs::path base) const {
    base = fs::weakly_canonical(base);
    for (auto b = base.begin(), p = canonical_uri_.begin(); b != base.end(); ++b, ++p) {
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

std::string RequestHandler::ParseMapToJson(const model::Map* map) {

    return json::serialize(json::value_from(*map));
}

}  // namespace http_handler
