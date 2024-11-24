#pragma once

#include <boost/asio/ip/tcp.hpp>

#include "player.h"
#include "model.h"

#include <chrono>
#include <string>
#include <string_view>

namespace serialization {
class ApplicationRepr;
} // namespace serialization

namespace app {

namespace net = boost::asio;

//**************************************************************
//GetMapUseCase

enum class GetMapErrorReason {
    mapNotFound
};

struct GetMapError {
    GetMapErrorReason reason;

    std::string what() const;
};

class GetMapUseCase {
public:
    explicit GetMapUseCase(const model::Game* game)
        : game_(game) {
    }

    const model::Map* GetMap(model::Map::Id map_id) const;

private:
    const model::Game* game_;
};

//**************************************************************
//ListMapsUseCase

class ListMapsUseCase {
public:
    explicit ListMapsUseCase(const model::Game* game)
        : game_(game) {
    }

    const model::Game::Maps& GetMapsList() const;

private:
    const model::Game* game_;
};

//**************************************************************
//ListPlayersUseCase

enum class ListPlayersErrorReason {
    unknownToken
};

struct ListPlayersError {
    ListPlayersErrorReason reason;

    std::string what() const;
};

class GetPlayersInfoUseCase {
public:
    GetPlayersInfoUseCase(const user::Players* players, const user::PlayerTokens* tokens)
        : players_(players)
        , tokens_(tokens) {
    }

    const model::GameSession::IdToDogIndex& GetPlayersList(std::string_view token) const;
    const model::GameSession* GetPlayerGameSession(std::string_view token) const;

private:
    const user::Players* players_;
    const user::PlayerTokens* tokens_;
};

//**************************************************************
//JoinGameUseCase

struct JoinGameResult {
    user::Token token;
    model::Dog::Id player_id;
};

enum class JoinGameErrorReason {
    invalidName,
    invalidMap
};

struct JoinGameError {
    JoinGameErrorReason reason;

    std::string what() const;
};

class JoinGameUseCase {
public:
    JoinGameUseCase(model::Game* game, user::Players* players, user::PlayerTokens* tokens)
        : game_(game)
        , players_(players)
        , tokens_(tokens) {
    }

    JoinGameResult JoinGame(const std::string& user_name, const std::string& map_id);

private:
    model::Game* game_;
    user::Players* players_;
    user::PlayerTokens* tokens_;
};

//**************************************************************
//ManageDogActionsUseCase

class ManageDogActionsUseCase {
public:
    ManageDogActionsUseCase(user::Players* players, user::PlayerTokens* tokens)
        : players_(players)
        , tokens_(tokens) {
    }

    bool MoveDog(std::string_view token, std::string_view move);

private:
    user::Players* players_;
    user::PlayerTokens* tokens_;
};

//**************************************************************
//ProcessTickUseCase

class ProcessTickUseCase {
public:
    explicit ProcessTickUseCase(model::Game* game)
        : game_(game) {
    }

    void ProcessTick(std::int64_t tick);

private:
    model::Game* game_;
};

//**************************************************************
//ApplicationListener Interface

class ApplicationListener {
public:
    virtual void OnTick(std::chrono::milliseconds delta) = 0;

protected:
    ~ApplicationListener() = default;
};

//**************************************************************
//Application

class Application {
public:
    friend class serialization::ApplicationRepr;

    explicit Application(model::Game* game)
        : game_(game) {
    }

    const model::Game::Maps& ListMaps() const;
    const model::Map* FindMap(model::Map::Id map_id) const;
    const model::GameSession* GetPlayerGameSession(std::string_view token) const;
    const model::GameSession::IdToDogIndex& ListPlayers(std::string_view token) const;
    JoinGameResult JoinGame(const std::string& user_name, const std::string& map_id);
    bool MoveDog(std::string_view token, std::string_view move);
    void ProcessTick(std::int64_t tick);

    bool IsTokenValid(std::string_view token) const;
    void SetListener(ApplicationListener* listener);

private:
    model::Game* game_;
    user::Players players_;
    user::PlayerTokens tokens_;

    ApplicationListener* listener_ = nullptr;

    // create all scenario below
    GetMapUseCase get_map_use_case_{game_};
    ListMapsUseCase list_maps_use_case_{game_};
    GetPlayersInfoUseCase get_players_info_use_case_{&players_, &tokens_};
    JoinGameUseCase join_game_use_case_{game_, &players_, &tokens_};
    ManageDogActionsUseCase manage_dog_actions_use_case_{&players_, &tokens_};
    ProcessTickUseCase process_tick_use_case_{game_};
};

} // namespace app
