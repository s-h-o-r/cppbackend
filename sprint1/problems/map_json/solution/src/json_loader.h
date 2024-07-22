#pragma once

#include <boost/json.hpp>
#include <filesystem>

#include "model.h"

namespace json = boost::json;

namespace json_loader {

model::Road PrepareRoad(json::value& road_info);

model::Building PrepareBuilding(json::value& building_info);

model::Office PrepareOffice(json::value& office_info);

model::Map PrepareMap(json::value& map_info);

model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
