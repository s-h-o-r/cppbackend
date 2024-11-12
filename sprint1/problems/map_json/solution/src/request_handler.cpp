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

std::string RequestHandler::ParseMapToJson(const model::Map* map) {

    return json::serialize(json::value_from(*map));
}

}  // namespace http_handler
