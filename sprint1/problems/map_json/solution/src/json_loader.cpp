#include "json_loader.h"
#include <fstream>
#include <iostream>
#include <string>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace json_loader {

using namespace std::literals;
namespace sys = boost::system;

model::Road PrepareRoad(json::object& road_info) {
    model::Point start{json::value_to<model::Coord>(road_info.at("x0"s)),
        json::value_to<model::Coord>(road_info.at("y0"s))};
    if (road_info.if_contains("x1"s)) {
        return model::Road(model::Road::HORIZONTAL, start,
                           json::value_to<model::Coord>(road_info.at("x1"s)));
    } else {
        return model::Road(model::Road::VERTICAL, start,
                           json::value_to<model::Coord>(road_info.at("y1"s)));
    }
}

model::Building PrepareBuilding(json::object& building_info) {
    model::Point point{json::value_to<model::Coord>(building_info.at("x"s)),
        json::value_to<model::Coord>(building_info.at("y"s))};
    model::Size size{json::value_to<model::Dimension>(building_info.at("w"s)),
        json::value_to<model::Dimension>(building_info.at("h"s))};

    return model::Building{{point, size}};
}

model::Office PrepareOffice(json::object& office_info) {
    model::Point point{json::value_to<model::Coord>(office_info.at("x"s)),
        json::value_to<model::Coord>(office_info.at("y"s))};
    model::Offset offset{json::value_to<model::Coord>(office_info.at("offsetX"s)),
        json::value_to<model::Coord>(office_info.at("offsetY"s))};

    std::string id_str(json::value_to<std::string>(office_info.at("id"s)));

    return {model::Office::Id{id_str}, point, offset};
}

model::Map PrepareMap(json::object& map_info) {
    model::Map map(model::Map::Id(json::value_to<std::string>(map_info.at("id"s))),
                   json::value_to<std::string>(map_info.at("name"s)));

    json::array roads = map_info.at("roads"s).as_array();
    std::cout << "Roads"sv << std::endl;
    for (auto it = roads.begin(); it != roads.end(); ++it) {
        map.AddRoad(PrepareRoad(it->as_object()));
    }

    json::array buildings = map_info.at("buildings"s).as_array();
    std::cout << "Buildings"sv << std::endl;
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        map.AddBuilding(PrepareBuilding(it->as_object()));
    }

    json::array offices = map_info.at("offices"s).as_array();
    std::cout << "Offices"sv << std::endl;
    for (auto it = offices.begin(); it != offices.end(); ++it) {
        map.AddOffice(PrepareOffice(it->as_object()));
    }

    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::ifstream json(json_path);
    if (!json.is_open()) {
        throw std::runtime_error("Cannot open the file.");
    }

    std::string json_data((std::istreambuf_iterator<char>(json)),
                          std::istreambuf_iterator<char>());

    sys::error_code ec;

    json::value game_info{json::parse(json_data, ec)};

    if (ec) {
        throw std::logic_error(ec.what());
    }

    /*
    json::array maps = game_info.at("maps"s).as_array();
    std::cout << "Maps"sv << std::endl;
*/

    for (const auto& map : value.at("maps").as_array())
    {
        game.AddMap(PrepareMap(map.as_object()));
    }

    /*
    for (auto it = maps.begin(); it != maps.end(); ++it) {
        game.AddMap(PrepareMap(it->as_object()));
    }
*/
    return game;
}

}  // namespace json_loader
