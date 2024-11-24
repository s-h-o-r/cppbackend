#pragma once
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>

#include "app.h"
#include "geom.h"
#include "model.h"
#include "player.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, model::Loot& obj, [[maybe_unused]] const unsigned version) {
    ar& *obj.id;
    ar& obj.type;
    ar& obj.point;
}

}  // namespace model

namespace serialization {

class BagRepr {
public:
    BagRepr() = default;

    explicit BagRepr(const game_obj::Bag<model::Loot>& bag)
        : loot_(bag.GetAllLoot())
        , capacity_(bag.GetCapacity()) {
    }

    [[nodiscard]] game_obj::Bag<model::Loot> Restore() const {
        game_obj::Bag<model::Loot> bag{capacity_};
        for (const model::Loot& item : loot_) {
            bag.PickUpLoot(item);
        }
        return bag;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& loot_;
        ar& capacity_;
    }

private:
    std::vector<model::Loot> loot_;
    size_t capacity_ = 0;
};

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetPosition())
        , speed_(dog.GetSpeed())
        , dir_(dog.GetDirection())
        , bag_(*dog.GetBag())
        , score_(dog.GetScore()) {
    }

    [[nodiscard]] std::shared_ptr<model::Dog> Restore() const {
        game_obj::Bag<model::Loot> bag = bag_.Restore();
        auto dog_ptr = std::make_shared<model::Dog>(id_, name_, pos_, speed_, bag.GetCapacity());
        dog_ptr->SetDirection(dir_);
        dog_ptr->AddScore(score_);
        auto* dog_bag = dog_ptr->GetBag();
        *dog_bag = std::move(bag);
        return dog_ptr;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& dir_;
        ar& bag_;
        ar& score_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    geom::Vec2D speed_;
    model::Direction dir_ = model::Direction::NORTH;
    BagRepr bag_;
    std::uint64_t score_ = 0;
};

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& session)
        : map_id_(session.GetMap()->GetId())
        , next_dog_id_(session.GetNextDogId())
        , next_loot_id_(session.GetNextLootId())
    {
        for (const auto& [_, dog] : session.GetDogs()) {
            dogs_.push_back(DogRepr(*dog));
        }

        for (const auto& [_, loot_sptr] : session.GetAllLoot()) {
            loot_.push_back(loot_sptr);
        }
    }

    [[nodiscard]] std::shared_ptr<model::GameSession> Restore(const model::Game* game) const {
        auto session = std::make_shared<model::GameSession>(game->FindMap(map_id_), game->IsDogSpawnRandom(), game->GetLootConfig());

        model::GameSession::IdToDogIndex dog_index;
        for (const DogRepr& dog_repr : dogs_) {
            auto dog = dog_repr.Restore();
            dog_index[dog->GetId()] = dog;
        }

        model::GameSession::IdToLootIndex loot_index;
        for (auto& loot : loot_) {
            loot_index[loot->id] = loot;
        }
        session->Restore(std::move(dog_index), next_dog_id_, std::move(loot_index), next_loot_id_);
        return session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& dogs_;
        ar& next_dog_id_;
        ar& loot_;
        ar& next_loot_id_;
    }

private:
    model::Map::Id map_id_ = model::Map::Id{""};
    std::vector<DogRepr> dogs_;
    std::uint32_t next_dog_id_ = 0;
    std::vector<std::shared_ptr<model::Loot>> loot_;
    std::uint32_t next_loot_id_ = 0;
};

class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(const model::Game& game) {
        for (const auto& [map_id, sessions] : game.GetAllSessions()) {
            for (const auto& session : sessions) {
                sessions_.push_back(GameSessionRepr(*session));
            }
        }
    }

    void Restore(model::Game* game) const {
        try {
            model::Game::SessionsByMaps sessions;
            for (auto& session_repr : sessions_) {
                auto session_ptr = session_repr.Restore(game);
                sessions[session_ptr->GetMapId()].push_back(session_ptr);
            }
            game->RestoreSessions(std::move(sessions));
        } catch (...) {
            throw std::logic_error("imposible to restore game for this configuration");
        }
        return game;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& sessions_;
    }

private:
    std::vector<GameSessionRepr> sessions_;
};

struct PlayerRepr {
    model::Map::Id map_id_ = model::Map::Id{""};
    model::Dog::Id dog_id_ = model::Dog::Id{0u};

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& *map_id_;
        ar& *dog_id_;
    }
};

class PlayersRepr {
public:
    PlayersRepr() = default;
    
    explicit PlayersRepr(const user::Players& players) {
        for (const auto& player : players.GetAllPlayers()) {
            players_.push_back({player->GetGameSession()->GetMapId(), player->GetDog()->GetId()});
        }
    }

    user::Players Restore(model::Game* game) const {
        user::Players restored_players;
        for (const PlayerRepr& player : players_) {
            restored_players.Add(game->GetGameSession(player.map_id_)->GetDog(player.dog_id_),
                                 game->GetGameSession(player.map_id_));
        }
        return restored_players;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
    }
private:
    std::vector<PlayerRepr> players_;
};

class PlayerTokenRepr {
public:
    PlayerTokenRepr() = default;

    explicit PlayerTokenRepr(const user::PlayerTokens& player_tokens) {
        for (const auto& [token, player_ptr] : player_tokens.token_to_player_) {
            auto res = token_to_player_.emplace(*token, PlayerRepr{player_ptr->GetGameSession()->GetMapId(), player_ptr->GetDog()->GetId()});
            if (!res.second) {
                throw std::logic_error("trying to emplace duplicated token");
            }
        }
    }

    user::PlayerTokens Restore(user::Players* players) const {
        user::PlayerTokens restored_player_tokens;
        for (const auto& [token, player_repr] : token_to_player_) {
            restored_player_tokens.token_to_player_.emplace(token, players->FindByDogIdAndMapId(player_repr.dog_id_,
                                                                                                player_repr.map_id_));
        }
        return restored_player_tokens;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& token_to_player_;
    }
private:
    std::unordered_map<std::string, PlayerRepr> token_to_player_;
};

class ApplicationRepr {
public:
    ApplicationRepr() = default;

    explicit ApplicationRepr(const app::Application& app)
        : players_(app.players_)
        , player_tokens_(app.tokens_) {

    }

    void RestoreGame(model::Game* game) const {
        return game_repr_.Restore(game);
    }

    app::Application RestoreApp(model::Game* game) const {
        app::Application app{game};
        app.players_ = players_.Restore(game);
        app.tokens_ = player_tokens_.Restore(&app.players_);
        return app;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_;
        ar& player_tokens_;
    }

private:
    PlayersRepr players_;
    PlayerTokenRepr player_tokens_;
    GameRepr game_repr_;
};

class SerializationListener : public app::ApplicationListener {
public:
    explicit SerializationListener(std::chrono::milliseconds save_period)
    : save_period_(save_period) {
    }

    void OnTick(std::chrono::milliseconds delta) override {
        time_since_save_ += delta;
        if (time_since_save_ >= save_period_) {
            Serialize();
            save_period_ = std::chrono::milliseconds::zero();
        }
    }

private:
    std::chrono::milliseconds save_period_;
    std::chrono::milliseconds time_since_save_{0};

    void Serialize() const {
        
    }
};

}  // namespace serialization
