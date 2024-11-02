#include "model.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <stdexcept>

namespace model {
using namespace std::literals;

std::string DirectionToString(Direction dir) {
    switch (dir) {
        case Direction::NORTH:
            return "U"s;
        case Direction::SOUTH:
            return "D"s;
        case Direction::EAST:
            return "R"s;
        case Direction::WEST:
            return "L"s;
    }
    throw std::runtime_error("Unknown direction status in Dog class"s);
}

Road::Road(HorizontalTag, geom::Point start, geom::Coord end_x) noexcept
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, geom::Point start, geom::Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

geom::Point Road::GetStart() const noexcept {
    return start_;
}

geom::Point Road::GetEnd() const noexcept {
    return end_;
}

bool Road::IsDogOnRoad(geom::Point2D dog_point) const {
    double right_edge = GetRightEdge();
    double left_edge = GetLeftEdge();
    double upper_edge = GetUpperEdge();
    double bottom_edge = GetBottomEdge();

    return dog_point.x >= left_edge && dog_point.x <= right_edge
        && dog_point.y >= upper_edge && dog_point.y <= bottom_edge;
}

double Road::GetLeftEdge() const {
    return std::min(start_.x, end_.x) - 0.4;
}

double Road::GetRightEdge() const {
    return std::max(start_.x, end_.x) + 0.4;
}

double Road::GetUpperEdge() const {
    return std::min(start_.y, end_.y) - 0.4;
}

double Road::GetBottomEdge() const {
    return std::max(start_.y, end_.y) + 0.4;
}

const geom::Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

geom::Point Office::GetPosition() const noexcept {
    return position_;
}

geom::Offset Office::GetOffset() const noexcept {
    return offset_;
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

geom::Velocity Map::GetSpeed() const noexcept {
    return dog_speed_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

const std::vector<extra_data::LootType>& Map::GetLootTypes() const noexcept {
    return loot_types_;
}

void Map::SetDogSpeed(geom::Velocity speed) {
    dog_speed_ = speed;
}

void Map::AddRoad(Road&& road) {
    roads_.push_back(std::move(road));
    Road& new_road = roads_.back();
    if (new_road.IsVertical()) {
        vertical_road_index_[new_road.GetStart().x].push_back(&new_road);
    } else {
        horizontal_road_index_[new_road.GetStart().y].push_back(&new_road);
    }
}

void Map::AddBuilding(Building&& building) {
    buildings_.emplace_back(std::move(building));
}

void Map::AddOffice(Office&& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::AddLootType(extra_data::LootType&& loot_type) {
    loot_types_.emplace_back(std::move(loot_type));
}

geom::Point2D Map::GetRandomPoint() const {
    if (roads_.size() == 0) {
        throw std::logic_error("No roads on map to generate random road"s);
    }

    std::random_device random_device;
    std::mt19937 generator{random_device()};

    std::uniform_int_distribution int_dist(0, static_cast<int>(roads_.size() - 1));

    const Road& rand_road = roads_.at(int_dist(generator));

    if (rand_road.IsHorizontal()) {
        std::uniform_real_distribution double_dist(std::min(rand_road.GetStart().x, rand_road.GetEnd().x) + 0.0,
                                                   std::max(rand_road.GetStart().x, rand_road.GetEnd().x) + 0.0);
        return {static_cast<double>(double_dist(generator)), static_cast<double>(rand_road.GetStart().y)};
    } else {
        std::uniform_real_distribution double_dist(std::min(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0,
                                                   std::max(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0);
        return {static_cast<double>(rand_road.GetStart().x), static_cast<double>(double_dist(generator))};
    }
}

geom::Point2D Map::GetDefaultSpawnPoint() const {
    geom::Point first_road_start = roads_.front().GetStart();
    return {static_cast<double>(first_road_start.x),
            static_cast<double>(first_road_start.y)};
}

const Road* Map::GetVerticalRoad(geom::Point2D dog_point) const {
    geom::Point map_point = ConvertToMapPoint(dog_point);

    if (vertical_road_index_.contains(map_point.x)) {
        for (const Road* road : vertical_road_index_.at(map_point.x)) {
            if (road->IsDogOnRoad(dog_point)) {
                return road;
            }
        }
    }
    return nullptr;
}

const Road* Map::GetHorizontalRoad(geom::Point2D dog_point) const {
    geom::Point map_point = ConvertToMapPoint(dog_point);
    if (horizontal_road_index_.contains(map_point.y)) {
        for (const Road* road : horizontal_road_index_.at(map_point.y)) {
            if (road->IsDogOnRoad(dog_point)) {
                return road;
            }
        }
    }
    return nullptr;
}

std::vector<const Road*> Map::GetRelevantRoads(geom::Point2D dog_point) const {
    std::vector<const Road*> relevant_road;
    for (auto& road : roads_) {
        if (road.IsDogOnRoad(dog_point)) {
            relevant_road.push_back(&road);
        }
    }
    return relevant_road;
}

const std::string& Dog::GetName() const noexcept {
    return name_;
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}

void Dog::SetName(std::string_view name) {
    name_ = std::string(name);
}

void Dog::SetPosition(geom::Point2D new_pos) {
    pos_ = new_pos;
}

const geom::Point2D& Dog::GetPosition() const {
    return pos_;
}

void Dog::SetSpeed(geom::Speed new_speed) {
    speed_ = new_speed;
}

const geom::Speed& Dog::GetSpeed() const {
    return speed_;
}

void Dog::SetDirection(Direction new_dir) {
    dir_ = new_dir;
}

Direction Dog::GetDirection() const {
    return dir_;

}

void Dog::Stop() {
    speed_ = {0, 0};
}

bool Dog::IsStopped() const {
    return std::fabs(speed_.s_x) < std::numeric_limits<double>::epsilon()
        && std::fabs(speed_.s_y) < std::numeric_limits<double>::epsilon();
}

const Map::Id& GameSession::GetMapId() const {
    return map_->GetId();
}

const model::Map* GameSession::GetMap() const {
    return map_;
}

Dog* GameSession::AddDog(std::string_view name) {
    detail::ThreadChecker checker(counter_);

    geom::Speed default_speed = {0, 0};

    geom::Point2D start_point;
    if (random_dog_spawn_) {
        start_point = map_->GetRandomPoint();
    } else {
        start_point = map_->GetDefaultSpawnPoint();
    }

    geom::Point2D dog_pos = {static_cast<double>(start_point.x),
                                static_cast<double>(start_point.y)}; // map_->GetRandomDogPoint();

    auto dog = std::make_shared<Dog>(std::string(name), dog_pos, default_speed);
    auto dog_id = dog->GetId();
    dogs_.emplace(dog_id, std::move(dog));
    return dogs_.at(dog_id).get();
}

const GameSession::IdToDogIndex& GameSession::GetDogs() const {
    return dogs_;
}

const std::vector<Loot>& GameSession::GetAllLoot() const {
    return loot_;
}

void GameSession::UpdateState(std::int64_t tick) {
    UpdateDogsState(tick);
    GenerateLoot(tick);
}

void GameSession::UpdateDogsState(std::int64_t tick) {
    double ms_convertion = 0.001; // 1ms = 0.001s
    double tick_multy = static_cast<double>(tick) * ms_convertion;

    for (auto [_, dog] : dogs_) {
        if (dog->IsStopped()) {
            continue;
        }

        auto cur_dog_pos = dog->GetPosition();
        auto dog_speed = dog->GetSpeed();
        geom::Point2D new_dog_pos = {cur_dog_pos.x + (dog_speed.s_x * tick_multy),
            cur_dog_pos.y + (dog_speed.s_y * tick_multy)};

        auto relevant_road = map_->GetRelevantRoads(cur_dog_pos);
        if (relevant_road.empty()) {
            throw std::logic_error("invalid dog position");
        }

        geom::Point2D relevant_point = cur_dog_pos;
        bool stopped = true;

        for (auto road : relevant_road) {
            if (road->IsDogOnRoad(new_dog_pos)) {
                relevant_point = new_dog_pos;
                stopped = false;
                break;
            } else {

                switch (dog->GetDirection()) {
                    case Direction::NORTH:
                        if (road->GetUpperEdge() < relevant_point.y) {
                            relevant_point = {relevant_point.x, road->GetUpperEdge()};
                        }
                        break;
                    case Direction::SOUTH:
                        if (road->GetBottomEdge() > relevant_point.y) {
                            relevant_point = {relevant_point.x, road->GetBottomEdge()};
                        }
                        break;
                    case Direction::EAST:
                        if (road->GetRightEdge() > relevant_point.x) {
                            relevant_point = {road->GetRightEdge(), relevant_point.y};
                        }
                        break;
                    case Direction::WEST:
                        if (road->GetLeftEdge() < relevant_point.x) {
                            relevant_point = {road->GetLeftEdge(), relevant_point.y};
                        }
                        break;
                    default:
                        throw std::logic_error("unknown dog direction");
                        break;
                }
            }
        }
        dog->SetPosition(relevant_point);
        if (stopped) {
            dog->Stop();
        }
    }

    /*
     Где-то здесь имеется ошибка в реализации хранения индекса карт по координатам, из-за чего некоторые
     кейсы неправильно обрабатываются на некоторых платформах (на Mac работает, в Докере падает, тесты
     не проходит). Не разобрался, потому что не смог нормально запустить дебаггер на платформах, на которых
     эта реализация падает. Надо будет разобраться и сделать более быстрый варинат обновления сервера

    for (auto [_, dog] : dogs_) {
        if (dog->IsStopped()) {
            continue;
        }

        auto cur_dog_pos = dog->GetPosition();
        auto dog_speed = dog->GetSpeed();
        DogPoint new_dog_pos = {cur_dog_pos.x + (dog_speed.s_x * tick_multy),
            cur_dog_pos.y + (dog_speed.s_y * tick_multy)};

        const Road* vertical_road_with_dog = map_->GetVerticalRoad(cur_dog_pos);
        const Road* horizontal_road_with_dog = map_->GetHorizontalRoad(cur_dog_pos);

        assert((vertical_road_with_dog != nullptr || horizontal_road_with_dog != nullptr));

        if (vertical_road_with_dog != nullptr && vertical_road_with_dog->IsDogOnRoad(new_dog_pos)) {
            dog->SetPosition(new_dog_pos);
            return;
        }

        if (horizontal_road_with_dog != nullptr && horizontal_road_with_dog->IsDogOnRoad(new_dog_pos)) {
            dog->SetPosition(new_dog_pos);
            return;
        }

        switch (dog->GetDirection()) {
            case Direction::NORTH: {
                if (vertical_road_with_dog != nullptr) {
                    new_dog_pos = {cur_dog_pos.x, vertical_road_with_dog->GetUpperEdge()};
                } else if (horizontal_road_with_dog != nullptr) {
                    new_dog_pos = {cur_dog_pos.x, horizontal_road_with_dog->GetUpperEdge()};
                }
                break;
            }
            case Direction::SOUTH: {
                if (vertical_road_with_dog != nullptr) {
                    new_dog_pos = {cur_dog_pos.x, vertical_road_with_dog->GetBottomEdge()};
                } else if (horizontal_road_with_dog != nullptr) {
                    new_dog_pos = {cur_dog_pos.x, horizontal_road_with_dog->GetBottomEdge()};
                }
                break;
            }
            case Direction::WEST: {
                if (horizontal_road_with_dog != nullptr) {
                    new_dog_pos = {horizontal_road_with_dog->GetLeftEdge(), cur_dog_pos.y};
                } else if (vertical_road_with_dog != nullptr) {
                    new_dog_pos = {vertical_road_with_dog->GetLeftEdge(), cur_dog_pos.y};
                }
                break;
            }
            case Direction::EAST: {
                if (horizontal_road_with_dog != nullptr) {
                    new_dog_pos = {horizontal_road_with_dog->GetRightEdge(), cur_dog_pos.y};
                } else if (vertical_road_with_dog != nullptr) {
                    new_dog_pos = {vertical_road_with_dog->GetRightEdge(), cur_dog_pos.y};
                }
                break;
            }
        }
        dog->SetPosition(new_dog_pos);
        dog->Stop();
    }
     */
}

void GameSession::GenerateLoot(std::int64_t tick) {
    loot_gen::LootGenerator::TimeInterval time_interval(tick);
    unsigned loot_counter = loot_generator_.Generate(time_interval, static_cast<unsigned>(loot_.size()), static_cast<unsigned>(dogs_.size()));
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(0, static_cast<unsigned>(map_->GetLootTypes().size() - 1));
    for (; loot_counter != 0; --loot_counter) {
        loot_.push_back({static_cast<uint8_t>(dist(rng)) , map_->GetRandomPoint()});
    }
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(std::move(map));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const Map* Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return &maps_.at(it->second);
    }
    return nullptr;
}

GameSession& Game::StartGameSession(const Map* map) {
    using namespace std::chrono_literals;
    if (sessions_[map->GetId()].empty()) {
        loot_gen::LootGenerator::TimeInterval time_interval(static_cast<int>(loot_config_.period * 1000));
        sessions_[map->GetId()].push_back(std::make_shared<GameSession>(map, random_dog_spawn_,
                                                                        loot_gen::LootGenerator(time_interval, loot_config_.probability)));
    }
    return *sessions_[map->GetId()].back();
}

GameSession* Game::GetGameSession(Map::Id map_id) {
    if (sessions_[map_id].empty()) {
        return nullptr;
    }
    return sessions_[map_id].back().get();
}

void Game::TurnOnRandomSpawn() {
    random_dog_spawn_ = true;
}

void Game::TurnOffRandomSpawn() {
    random_dog_spawn_ = false;
}

void Game::SetDogSpeed(geom::Velocity default_speed) {
    default_dog_speed_ = default_speed;
}

geom::Velocity Game::GetDefaultGogSpeed() const noexcept {
    return default_dog_speed_;
}

void Game::SetLootConfig(double period, double probability) {
    loot_config_.period = period;
    loot_config_.probability = probability;
}

const LootConfig& Game::GetLootConfig() const {
    return loot_config_;
}

void Game::UpdateState(std::int64_t tick) {
    for (auto& [_, map_sessions] : sessions_) {
        for (auto session : map_sessions) {
            session->UpdateState(tick);
        }
    }
}
}  // namespace model
