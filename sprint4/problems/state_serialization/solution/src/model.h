#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <atomic>
#include <cmath>
#include <deque>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "collision_detector.h"
#include "extra_data.h"
#include "game_objects.h"
#include "geom.h"
#include "loot_generator.h"
#include "tagged.h"

/*namespace detail {
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
*/
namespace model {

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

    Road(HorizontalTag, geom::Point start, geom::Coord end_x) noexcept;
    Road(VerticalTag, geom::Point start, geom::Coord end_y) noexcept;

    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    geom::Point GetStart() const noexcept;
    geom::Point GetEnd() const noexcept;

    bool IsDogOnRoad(geom::Point2D dog_point) const;

    double GetLeftEdge() const;
    double GetRightEdge() const;
    double GetUpperEdge() const;
    double GetBottomEdge() const;

private:
    geom::Point start_;
    geom::Point end_;
};

class Building {
public:
    explicit Building(geom::Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const geom::Rectangle& GetBounds() const noexcept;

private:
    geom::Rectangle bounds_;};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, geom::Point position, geom::Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept;
    geom::Point GetPosition() const noexcept;
    geom::Offset GetOffset() const noexcept;
    double GetWidth() const noexcept;

private:
    Id id_;
    geom::Point position_;
    geom::Offset offset_;

    double width_ = 0.5;
};

class Map { // todo: сделать класс более надежным к ошибкам конфиг файла (вынести все деволтные атрибуты в качестве обязательных в конструктор)
            // пока же дефолтное значение скорости устанавливается через функцию
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::deque<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    double GetSpeed() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    const std::vector<extra_data::LootType>& GetLootTypes() const noexcept;
    size_t GetBagCapacity() const noexcept;
    unsigned GetLootScore(std::uint8_t loot_type) const noexcept;

    void SetDogSpeed(double speed);
    void AddRoad(Road&& road);
    void AddBuilding(Building&& building);
    void AddOffice(Office&& office);
    void AddLootType(extra_data::LootType&& loot_type, unsigned score);
    void SetBagCapacity(size_t bag_capacity);

    geom::Point2D GetRandomPoint() const;
    geom::Point2D GetDefaultSpawnPoint() const;

    const Road* GetVerticalRoad(geom::Point2D dog_point) const;
    const Road* GetHorizontalRoad(geom::Point2D dog_point) const;

    std::vector<const Road*> GetRelevantRoads(geom::Point2D dog_point) const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;
    using VerticalRoadIndex = std::unordered_map<geom::Coord, std::vector<const Road*>>;
    using HorizontalRoadIndex = std::unordered_map<geom::Coord, std::vector<const Road*>>;

    Id id_;
    std::string name_;
    double dog_speed_;
    size_t bag_capacity_;

    Roads roads_;
    VerticalRoadIndex vertical_road_index_;
    HorizontalRoadIndex horizontal_road_index_;

    Buildings buildings_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;

    std::vector<extra_data::LootType> loot_types_;
    std::unordered_map<std::uint8_t, unsigned> loot_type_to_score_;
};

namespace net = boost::asio;

struct Loot {
    using Id = util::Tagged<size_t, Loot>;

    Id id{-1};
    std::uint8_t type;
    geom::Point2D point;

    auto operator<=>(const Loot&) const = default;
};

class Dog {
public:
    using Id = util::Tagged<std::uint32_t, Dog>;

    explicit Dog(Id id, std::string name, geom::Point2D pos, geom::Vec2D speed,
                 size_t bag_capacity) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , pos_(pos)
        , prev_pos_(pos)
        , speed_(speed)
        , bag_(bag_capacity) {
    }

    auto operator<=>(const Dog&) const = default;

    const std::string& GetName() const noexcept;
    const Id& GetId() const noexcept;
    void SetName(std::string_view name);

    void SetPosition(geom::Point2D new_pos);
    const geom::Point2D& GetPosition() const;
    const geom::Point2D& GetPreviousPosition() const;
    void SetSpeed(geom::Vec2D new_speed);
    const geom::Vec2D& GetSpeed() const;
    void SetDirection(Direction new_dir);
    Direction GetDirection() const;
    double GetWidth() const noexcept;

    void Stop();
    bool IsStopped() const;

    game_obj::Bag<Loot>* GetBag();
    void AddScore(std::uint64_t score_to_add);
    std::uint64_t GetScore() const;

private:
    Id id_;
    std::string name_;
    geom::Point2D pos_;
    geom::Point2D prev_pos_;
    geom::Vec2D speed_;
    Direction dir_ = Direction::NORTH;
    double width_ = 0.6;

    game_obj::Bag<Loot> bag_;
    std::uint64_t score_ = 0;
};

struct LootConfig {
    double period = 0.;
    double probability = 0.;
};

class GameSession;

class LootOfficeDogProvider : public collision_detector::ItemGathererProvider {
public:
    size_t ItemsCount() const override;
    collision_detector::Item GetItem(size_t idx) const override;
    size_t GatherersCount() const override;
    collision_detector::Gatherer GetGatherer(size_t idx) const override;

    void PushBackLoot(const Loot* loot);
    void EraseLoot(size_t idx);
    const Loot* GetRawItemVal(size_t idx) const;
    void AddGatherer(Dog* gatherer);
    const Dog* GetDog(size_t idx) const;
    Dog* GetDog(size_t idx);


private:
    using LootId = size_t;

    std::vector<const Loot*> items_;
    std::vector<Dog*> gatherers_;
};

class GameSession {
public:
    using Id = util::Tagged<std::uint64_t, GameSession>;
    using DogIdHasher = util::TaggedHasher<Dog::Id>;
    using IdToDogIndex = std::unordered_map<Dog::Id, std::shared_ptr<Dog>, DogIdHasher>;

    using LootIdHasher = util::TaggedHasher<Loot::Id>;
    using IdToLootIndex = std::unordered_map<Loot::Id, std::shared_ptr<Loot>, LootIdHasher>;

    explicit GameSession(const Map* map, bool random_dog_spawn, const LootConfig& loot_config)
        : map_(map)
        , random_dog_spawn_(random_dog_spawn)
        , loot_generator_(loot_gen::LootGenerator::TimeInterval(static_cast<int>(loot_config.period * 1000)),
                          loot_config.probability) {
        if (map == nullptr) {
            throw std::runtime_error("Cannot open game session on empty map");
        }
    }

    GameSession(const GameSession&) = delete;
    GameSession operator=(const GameSession&) = delete;

    GameSession(GameSession&&) = default;

    const Map::Id& GetMapId() const;
    const model::Map* GetMap() const;
    Dog* AddDog(std::string_view name);
    const Dog* GetDog(Dog::Id id) const;
    Dog* GetDog(Dog::Id id);
    const IdToDogIndex& GetDogs() const;
    const IdToLootIndex& GetAllLoot() const;

    void EraseLoot(Loot::Id loot_id);

    void UpdateState(std::int64_t tick);

    std::uint32_t GetNextDogId() const;
    std::uint32_t GetNextLootId() const;

    void Restore(IdToDogIndex&& dogs, std::uint32_t next_dog_id, IdToLootIndex&& loot, std::uint32_t next_loot_id);

private:
    const Map* map_;
    IdToDogIndex dogs_;
    std::uint32_t next_dog_id_ = 0;
    bool random_dog_spawn_ = false;

    IdToLootIndex loot_;
    std::uint32_t next_loot_id_ = 0;
    loot_gen::LootGenerator loot_generator_;
    LootOfficeDogProvider items_gatherer_provider_;

    void UpdateDogsState(std::int64_t tick);
    void HandleCollisions();
    void GenerateLoot(std::int64_t tick);
};

class Game {
public:
    using Maps = std::vector<Map>;
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using SessionsByMaps = std::unordered_map<Map::Id, std::vector<std::shared_ptr<GameSession>>, MapIdHasher>;

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    const Map* FindMap(const Map::Id& id) const noexcept;

    GameSession& StartGameSession(const Map* map);
    const GameSession* GetGameSession(Map::Id map_id) const;
    GameSession* GetGameSession(Map::Id map_id);
    const SessionsByMaps& GetAllSessions() const;
    void RestoreSessions(SessionsByMaps&& restoring_sessions);
    /* на данном этапе сессия будет одна, но можно будет расширить
     функционал, если нужно будет ограничить количесто сессий на одной карте. Тогда вторым
     аргументом в функцию будет передаваться, например, Id сессии.
     На данном этапе мы создаем в векторе только одну сессию и берем ее с конца вектора
     */

    void TurnOnRandomSpawn();
    void TurnOffRandomSpawn();
    
    void SetDogSpeed(double default_speed);
    double GetDefaultGogSpeed() const noexcept;

    void SetLootConfig(double period, double probability);
    const LootConfig& GetLootConfig() const;

    bool IsDogSpawnRandom() const;

    void UpdateState(std::int64_t tick);

private:

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;

    double default_dog_speed_ = 1.;
    bool random_dog_spawn_ = false;

    LootConfig loot_config_;

    SessionsByMaps sessions_;
};

}  // namespace model
