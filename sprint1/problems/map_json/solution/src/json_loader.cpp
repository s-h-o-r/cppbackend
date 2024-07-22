#include "json_loader.h"
#include <fstream>
#include <string>

#define BOOST_BEAST_USE_STD_STRING_VIEW

namespace json_loader {

using namespace std::literals;

model::Road PrepareRoad(json::object& road_info) {

}

model::Building PrepareBuilding(json::object& building_info) {

}

model::Office PrepareOffice(json::object& office_info) {
    model::Point point{office_info.at("x"sv).as_int64(), 
        office_info.at("y"sv).as_int64()};
    model::Offset offset{office_info.at("offsetX"sv).as_int64(),
        office_info.at("offsetY"sv).as_int64()};

    auto& id = office_info.at("id"sv).as_string();
    std::string id_str(id.begin(), id.end());

    return {model::Office::Id{id_str}, point, offset};
}

model::Map PrepareMap(json::object& map_info) {
    auto& id = map_info.at("id"sv).get_string();
    auto& name = map_info.at("name"sv).get_string();
    model::Map map(model::Map::Id({id.begin(), id.end()}), 
                   {name.begin(), name.end()});

    json::array roads = map_info.at("roads"sv).as_array();
    for (auto it = roads.begin(); it != roads.end(); ++it) {
        map.AddRoad(PrepareRoad(*it));
    }

    json::array buildings = map_info.at("buildings"sv).as_array();
    for (auto it = buildings.begin(); it != buildings.end(); ++it) {
        map.AddBuilding(PrepareBuilding(*it));
    }

    json::array offices = map_info.at("offices"sv).as_array();
    for (auto it = offices.begin(); it != offices.end(); ++it) {
        map.AddOffice(PrepareOffice(*it));
    }

    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;

    std::fstream json(json_path);
    if (!json.is_open()) {
        throw std::runtime_error("Cannot open the file.");
    }

    std::string json_data((std::istreambuf_iterator<char>(json)),
                          std::istreambuf_iterator<char>());
    json::error_code ec;

    json::value game_info{json::parse(json_data, ec)};

    json::array& maps = game_info.as_array();

    for (auto it = maps.begin(); it != maps.end(); ++it) {
        game.AddMap(PrepareMap(*it));
    }

    return game;
}

}  // namespace json_loader
