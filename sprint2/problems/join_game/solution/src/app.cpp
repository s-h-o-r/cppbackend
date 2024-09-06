#include "app.h"

namespace app {

using namespace std::literals;

std::string GetMapError::what() const {
    switch (reason) {
        case GetMapErrorReason::mapNotFound:
            return "Map not found"s;
    }
}

const model::Map* GetMapUseCase::GetMap(model::Map::Id map_id) const {
    const model::Map* map = game_->FindMap(map_id);
    if (map == nullptr) {
        throw GetMapError{GetMapErrorReason::mapNotFound};
    }
    return map;
}

const model::Game::Maps& ListMapsUseCase::GetMapsList() const {
    return game_->GetMaps();
}

std::string ListPlayersError::what() const {
    switch (reason) {
        case ListPlayersErrorReason::unknownToken:
            return "Player token has not been found"s;
    }
}

const model::GameSession::IdToDogIndex& ListPlayersUseCase::GetPlayersList(std::string_view token) const {
    const user::Player* player = tokens_->FindPlayerByToken(user::Token{std::string(token)});
    if (player == nullptr) {
        throw ListPlayersError{ListPlayersErrorReason::unknownToken};
    }
    return player->GetGameSession()->GetDogs();
}


std::string JoinGameError::what() const {
    switch (reason) {
        case JoinGameErrorReason::invalidMap:
            return "Map not found"s;
        case JoinGameErrorReason::invalidName:
            return "Invalid name"s;
    }
}

JoinGameResult JoinGameUseCase::JoinGame(const std::string& user_name, const std::string& map_id) {
    if (user_name.empty()) {
        throw JoinGameError{JoinGameErrorReason::invalidName};
    }

    const model::Map* map = game_->FindMap(model::Map::Id{map_id});

    if (!map) {
        throw JoinGameError{JoinGameErrorReason::invalidName};
    }

    model::GameSession* session = game_->GetGameSession(model::Map::Id{map_id});
    if (session == nullptr) {
        session = &game_->StartGameSession(ioc_, map);
    }

    model::Dog* dog = session->AddDog(user_name);
    user::Token player_token = tokens_->AddPlayer(&players_->Add(dog, session));

    return {player_token, dog->GetId()};
}

const model::Game::Maps& Application::ListMaps() const {
    return list_maps_use_case_.GetMapsList();
}

const model::Map* Application::FindMap(model::Map::Id map_id) const {
    return get_map_use_case_.GetMap(map_id);
}

const model::GameSession::IdToDogIndex& Application::ListPlayers(std::string_view token) const {
    return list_players_use_case_.GetPlayersList(token);
}

JoinGameResult Application::JoinGame(const std::string& user_name, const std::string& map_id) {
    return join_game_use_case_.JoinGame(user_name, map_id);
}

} // namespace app
