#include "request_handler.h"

namespace http_handler {

namespace details {

/*
* На будущее: можно реализовать парсинг через перегрузку tag_invoke функцию
* всех отдельных элементов карты, а в основной функции парсинга использвоать
* json::value_from(). Пока оставлю как есть.

void tag_invoke(json::value_from_tag, json::value& jv, const model::Map& map) {

}
*/

} // namespace http_handler::details


std::string RequestHandler::ParseMapToJson(const model::Map* map) {
    json::object map_info_json;
    map_info_json["id"] = *(map->GetId());
    map_info_json["name"] = map->GetName();

    const auto& roads = map->GetRoads();
    map_info_json["roads"].emplace_array();
    for (const model::Road& road : roads) {
        auto start = road.GetStart();
        auto end = road.GetEnd();
        if (road.IsVertical()) {
            map_info_json["roads"].as_array().push_back(
            {
                {"x0", start.x}, {"y0", start.y}, {"y1", end.y}
            });
        } else {
            map_info_json["roads"].as_array().push_back(
            {
                {"x0", start.x}, {"y0", start.y}, {"x1", end.x}
            });
        }
    }

    const auto& buildings = map->GetBuildings();
    map_info_json["buildings"].emplace_array();
    for (const model::Building& building : buildings) {
        auto bounds = building.GetBounds();
        map_info_json["buildings"].as_array().push_back({
            {"x", bounds.position.x}, {"y", bounds.position.y}, 
            {"w", bounds.size.width}, {"h", bounds.size.height}
        });
    }
 
    const auto& offices = map->GetOffices();
    map_info_json["offices"].emplace_array();
    for (const model::Office& office : offices) {
        auto position = office.GetPosition();
        auto offset = office.GetOffset();

        map_info_json["offices"].as_array().push_back({
            {"id", *office.GetId()}, {"x", position.x}, {"y", position.y},
            {"offsetX", offset.dx}, {"offsetY", offset.dy}
        });
    }
    return json::serialize(json::value(std::move(map_info_json)));
}

}  // namespace http_handler
