#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <cmath>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tagged.h"


#include <atomic>

namespace detail {
class ThreadChecker {
public:
    explicit ThreadChecker(std::atomic_int& counter)
        : counter_{counter} {
    }

    ThreadChecker(const ThreadChecker&) = delete;
    ThreadChecker& operator=(const ThreadChecker&) = delete;

    ~ThreadChecker() {
        // assert выстрелит, если между вызовом конструктора и деструктора
        // значение expected_counter_ изменится
        assert(expected_counter_ == counter_);
    }

private:
    std::atomic_int& counter_;
    int expected_counter_ = ++counter_;
};
}

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

using DogCoord = double;
using Velocity = double;

struct DogPoint {
    DogCoord x, y;

    Point ConvertToMapPoint() const {
        return {static_cast<Coord>(std::round(x)),
            static_cast<Coord>(std::round(y))};
    }
};

struct Speed {
    Velocity s_x, s_y;
};

enum class Direction {
    NORTH, SOUTH, WEST, EAST
};

std::string DirectionToString(Direction dir);

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    constexpr static double WIDTH_ROAD_COEF_MINUS = -0.4;
    constexpr static double WIDTH_ROAD_COEF_PLUS = 0.4;

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;

    bool IsPointOnRoad(Point point) const;

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept;

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept;
    Point GetPosition() const noexcept;
    Offset GetOffset() const noexcept;

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    Velocity GetSpeed() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;

    void SetDogSpeed(Velocity speed);
    void AddRoad(Road&& road);
    void AddBuilding(Building&& building);
    void AddOffice(Office&& office);

    DogPoint GetRandomDogPoint() const;

    const Road* GetVerticalRoad(DogPoint dog_point) const;
    const Road* GetHorizontalRoad(DogPoint dog_point) const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    using VerticalRoadIndex = std::unordered_map<Coord, std::vector<const Road*>>;
    using HorizontalRoadIndex = std::unordered_map<Coord, std::vector<const Road*>>;


    Id id_;
    std::string name_;
    Velocity dog_speed_;

    Roads roads_;
    VerticalRoadIndex vertical_road_index_;
    HorizontalRoadIndex horizontal_road_index_;

    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

namespace net = boost::asio;

class Dog {
public:
    using Id = util::Tagged<std::uint32_t, Dog>;

    explicit Dog(std::string name, DogPoint pos, Speed speed) noexcept
        : id_(next_dog_id++)
        , name_(name)
        , pos_(pos)
        , speed_(speed) {
            detail::ThreadChecker varname(counter_);
    }

    const std::string& GetName() const noexcept;
    const Id& GetId() const noexcept;
    void SetName(std::string_view name);

    void SetPosition(DogPoint new_pos);
    const DogPoint& GetPosition() const;
    void SetSpeed(Speed new_speed);
    const Speed& GetSpeed() const;
    void SetDirection(Direction new_dir);
    Direction GetDirection() const;

    void Stop();
    bool IsStopped() const;
    
private:
    Id id_;
    std::string name_;
    DogPoint pos_;
    Speed speed_;
    Direction dir_ = Direction::NORTH;

    std::atomic_int counter_{0};
    static inline std::uint32_t next_dog_id = 0;
};

class GameSession {
public:
    using Id = util::Tagged<std::uint64_t, GameSession>;
    using DogIdHasher = util::TaggedHasher<Dog::Id>;
    using IdToDogIndex = std::map<Dog::Id, std::shared_ptr<Dog>>;

    explicit GameSession(const Map* map)
        : map_(map) {
            if (map == nullptr) {
                throw std::runtime_error("Cannot open game session on empty map");
            }
    }

    GameSession(const GameSession&) = delete;
    GameSession operator=(const GameSession&) = delete;

    const Map::Id& GetMapId() const;
    const model::Map* GetMap() const;
    Dog* AddDog(std::string_view name);
    const IdToDogIndex& GetDogs() const;

    void UpdateState(std::uint64_t tick);

private:
    const Map* map_;
    IdToDogIndex dogs_;

    std::atomic_int counter_{0};
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;

    GameSession& StartGameSession(const Map* map);
    GameSession* GetGameSession(Map::Id map_id);
    /* на данном этапе сессия будет одна, но можно будет расширить
     функционал, если нужно будет ограничить количесто сессий на одной карте. Тогда вторым
     аргументом в функцию будет передаваться, например, Id сессии.
     На данном этапе мы создаем в векторе только одну сессию и берем ее с конца вектора
     */

    void SetDogSpeed(Velocity default_speed);
    Velocity GetDefaultGogSpeed() const noexcept;

    void UpdateState(std::uint64_t tick);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using SessionsByMaps = std::unordered_map<Map::Id, std::vector<std::unique_ptr<GameSession>>, MapIdHasher>;

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    Velocity default_dog_speed_ = 1.;

    SessionsByMaps sessions_;
};

}  // namespace model
