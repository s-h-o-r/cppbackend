#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json = boost::json;

namespace json_loader {

model::Road PrepareRoad(json::object& road_info);

model::Building PrepareBuilding(json::object& building_info);

model::Office PrepareOffice(json::object& office_info);

model::Map PrepareMap(json::object& map_info);

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
