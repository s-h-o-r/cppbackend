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

Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
    : start_{start}
    , end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}
    , end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

bool Road::IsPointOnRoad(Point point) const {
    if (IsVertical()) {
        return start_.x == point.x
            && point.y >= std::min(start_.y, end_.y)
            && point.y <= std::max(start_.y, end_.y);
    } else {
        return start_.y == point.y
            && point.x >= std::min(start_.x, end_.x)
            && point.x <= std::max(start_.x, end_.x);
    }
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Point Office::GetPosition() const noexcept {
    return position_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

Velocity Map::GetSpeed() const noexcept {
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

void Map::SetDogSpeed(Velocity speed) {
    dog_speed_ = speed;
}

void Map::AddRoad(Road&& road) {
    roads_.emplace_back(std::move(road));
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

DogPoint Map::GetRandomDogPoint() const {
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
        return {static_cast<DogCoord>(double_dist(generator)), static_cast<DogCoord>(rand_road.GetStart().y)};
    } else {
        std::uniform_real_distribution double_dist(std::min(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0,
                                                   std::max(rand_road.GetStart().y, rand_road.GetEnd().y) + 0.0);
        return {static_cast<DogCoord>(rand_road.GetStart().x), static_cast<DogCoord>(double_dist(generator))};
    }
}

const Road* Map::GetVerticalRoad(DogPoint dog_point) const {
    Point map_point = dog_point.ConvertToMapPoint();

    double difference_x = (double)map_point.x - dog_point.x;
    double difference_y = (double)map_point.y- dog_point.y;

    if (difference_x > Road::WIDTH_ROAD_COEF_MINUS
        && difference_x < Road::WIDTH_ROAD_COEF_PLUS
        && vertical_road_index_.contains(map_point.x)) {
        for (const Road* road : vertical_road_index_.at(map_point.x)) {
            if (road->GetStart().y == map_point.y || road->GetEnd().y == map_point.y) {
                double min_dog_coord = std::min(road->GetStart().y, road->GetEnd().y) - 0.4;
                double max_dog_coord = std::max(road->GetStart().y, road->GetEnd().y) + 0.4;
                if (dog_point.y < min_dog_coord || max_dog_coord < dog_point.y) {
                    continue;
                }
            }

            if (road->IsPointOnRoad(map_point)) {
                return road;
            }
        }
    }
    return nullptr;
}

const Road* Map::GetHorizontalRoad(DogPoint dog_point) const {
    Point map_point = dog_point.ConvertToMapPoint();

    double difference_x = (double)map_point.x - dog_point.x;
    double difference_y = (double)map_point.y- dog_point.y;

    if (difference_y > Road::WIDTH_ROAD_COEF_MINUS
        && difference_y < Road::WIDTH_ROAD_COEF_PLUS
        && horizontal_road_index_.contains(map_point.y)) {
        for (const Road* road : horizontal_road_index_.at(map_point.y)) {
            if (road->GetStart().x == map_point.x || road->GetEnd().x == map_point.x) {
                double min_dog_coord = std::min(road->GetStart().x, road->GetEnd().x) - 0.4;
                double max_dog_coord = std::max(road->GetStart().x, road->GetEnd().x) + 0.4;
                if (dog_point.x < min_dog_coord || max_dog_coord < dog_point.x) {
                    continue;
                }
            }

            if (road->IsPointOnRoad(map_point)) {
                return road;
            }
        }
    }
    return nullptr;
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

void Dog::SetPosition(DogPoint new_pos) {
    pos_ = new_pos;
}

const DogPoint& Dog::GetPosition() const {
    return pos_;
}

void Dog::SetSpeed(Speed new_speed) {
    speed_ = new_speed;
}

const Speed& Dog::GetSpeed() const {
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
    assert(map_ != nullptr);
    return map_->GetId();
}

const model::Map* GameSession::GetMap() const {
    return map_;
}

Dog* GameSession::AddDog(std::string_view name) {
    detail::ThreadChecker checker(counter_);

    Speed default_speed = {0, 0};

    Point start_point = map_->GetRoads().front().GetStart();
    model::DogPoint dog_pos = {static_cast<DogCoord>(start_point.x),
                                static_cast<DogCoord>(start_point.y)}; // map_->GetRandomDogPoint();

    auto dog = std::make_shared<Dog>(std::string(name), dog_pos, default_speed);
    auto dog_id = dog->GetId();
    dogs_.emplace(dog_id, std::move(dog));
    return dogs_.at(dog_id).get();
}

const GameSession::IdToDogIndex& GameSession::GetDogs() const {
    return dogs_;
}

void GameSession::UpdateState(std::uint64_t tick) {
    double ms_convertion = 0.001; // 1ms = 0.001s
    double tick_multy = tick * ms_convertion;

    for (auto& [_, dog] : dogs_) {
        if (dog->IsStopped()) {
            continue;;
        }

        auto cur_dog_pos = dog->GetPosition();
        auto dog_speed = dog->GetSpeed();
        DogPoint new_dog_pos = {cur_dog_pos.x + (dog_speed.s_x * tick_multy),
                            cur_dog_pos.y + (dog_speed.s_y * tick_multy)};

        const Road* vertical_road_with_dog = map_->GetVerticalRoad(dog->GetPosition());
        const Road* horizontal_road_with_dog = map_->GetHorizontalRoad(dog->GetPosition());

        const Road* new_vertical_road_with_dog = map_->GetVerticalRoad(new_dog_pos);
        const Road* new_horizontal_road_with_dog = map_->GetHorizontalRoad(new_dog_pos);

        if (!((vertical_road_with_dog != nullptr && vertical_road_with_dog == new_vertical_road_with_dog)
               || (horizontal_road_with_dog != nullptr && horizontal_road_with_dog == new_horizontal_road_with_dog))) {

            switch (dog->GetDirection()) {
                case Direction::NORTH: {
                    if (vertical_road_with_dog != nullptr) {
                        Coord edge_road_coord = std::min(vertical_road_with_dog->GetStart().y,
                                                         vertical_road_with_dog->GetEnd().y);
                        new_dog_pos = {cur_dog_pos.x, edge_road_coord - 0.4};
                    } else if (horizontal_road_with_dog != nullptr) {
                        Coord y_coord = cur_dog_pos.ConvertToMapPoint().y;
                        new_dog_pos = {cur_dog_pos.x, y_coord - 0.4};
                    }
                    break;
                }
                case Direction::SOUTH: {
                    if (vertical_road_with_dog != nullptr) {
                        Coord edge_road_coord = std::max(vertical_road_with_dog->GetStart().y,
                                                         vertical_road_with_dog->GetEnd().y);
                        new_dog_pos = {cur_dog_pos.x, edge_road_coord + 0.4};
                    } else if (horizontal_road_with_dog != nullptr) {
                        Coord y_coord = cur_dog_pos.ConvertToMapPoint().y;
                        new_dog_pos = {cur_dog_pos.x, y_coord + 0.4};
                    }
                    break;
                }
                case Direction::WEST: {
                    if (horizontal_road_with_dog != nullptr) {
                        Coord edge_road_coord = std::min(horizontal_road_with_dog->GetStart().x,
                                                         horizontal_road_with_dog->GetEnd().x);
                        new_dog_pos = {edge_road_coord - 0.4, cur_dog_pos.y};
                    } else if (vertical_road_with_dog != nullptr) {
                        Coord x_coord = cur_dog_pos.ConvertToMapPoint().x;
                        new_dog_pos = {x_coord - 0.4, cur_dog_pos.y};
                    }
                    break;
                }
                case Direction::EAST: {
                    if (horizontal_road_with_dog != nullptr) {
                        Coord edge_road_coord = std::max(horizontal_road_with_dog->GetStart().x,
                                                         horizontal_road_with_dog->GetEnd().x);
                        new_dog_pos = {edge_road_coord + 0.4, cur_dog_pos.y};
                    } else if (vertical_road_with_dog != nullptr) {
                        Coord x_coord = cur_dog_pos.ConvertToMapPoint().x;
                        new_dog_pos = {x_coord + 0.4, cur_dog_pos.y};
                    }
                    break;
                }
                default:
                    throw std::runtime_error("unknown direction for dog");
            }
            dog->Stop();
        }
        dog->SetPosition(new_dog_pos);
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
    if (sessions_[map->GetId()].empty()) {
        sessions_[map->GetId()].push_back(std::make_unique<GameSession>(map));
    }
    return *sessions_[map->GetId()].back();
}

GameSession* Game::GetGameSession(Map::Id map_id) {
    if (sessions_[map_id].empty()) {
        return nullptr;
    }
    return sessions_[map_id].back().get();
}

void Game::SetDogSpeed(Velocity default_speed) {
    default_dog_speed_ = default_speed;
}

Velocity Game::GetDefaultGogSpeed() const noexcept {
    return default_dog_speed_;
}

void Game::UpdateState(std::uint64_t tick) {
    for (auto& [_, map_sessions] : sessions_) {
        for (auto& session : map_sessions) {
            session->UpdateState(tick);
        }
    }
}
}  // namespace model
