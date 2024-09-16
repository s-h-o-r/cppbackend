#include "app.h"

namespace app {

using namespace std::literals;

std::string GetMapError::what() const {
    switch (reason) {
        case GetMapErrorReason::mapNotFound:
            return "Map not found"s;
    }
    throw std::runtime_error("Unknown GetMapError");
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
    throw std::runtime_error("Unknown ListPlayersError");
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
    throw std::runtime_error("Unknown JoinGameError");
}

JoinGameResult JoinGameUseCase::JoinGame(const std::string& user_name, const std::string& map_id) {
    if (user_name.empty()) {
        throw JoinGameError{JoinGameErrorReason::invalidName};
    }

    const model::Map* map = game_->FindMap(model::Map::Id{map_id});

    if (!map) {
        throw JoinGameError{JoinGameErrorReason::invalidMap};
    }

    model::GameSession* session = game_->GetGameSession(model::Map::Id{map_id});
    if (session == nullptr) {
        session = &game_->StartGameSession(map);
    }

    model::Dog* dog = session->AddDog(user_name);
    user::Token player_token = tokens_->AddPlayer(&players_->Add(dog, session));

    return {player_token, dog->GetId()};
}

bool ManageDogActionsUseCase::MoveDog(std::string_view token, std::string_view move) {
    user::Player* player = tokens_->FindPlayerByToken(user::Token{std::string(token)});
    if (player == nullptr) {
        throw ListPlayersError{ListPlayersErrorReason::unknownToken};
    }

    model::Velocity map_speed = player->GetGameSession()->GetMap()->GetSpeed();
    model::Dog* dog = player->GetDog();

    if (move.empty()) {
        dog->Stop();
    } else if (move == "L"sv) {
        dog->SetSpeed({-map_speed, 0});
        dog->SetDirection(model::Direction::WEST);
    } else if (move == "R"sv) {
        dog->SetSpeed({map_speed, 0});
        dog->SetDirection(model::Direction::EAST);
    } else if (move == "U"sv) {
        dog->SetSpeed({0, -map_speed});
        dog->SetDirection(model::Direction::NORTH);
    } else if (move == "D"sv) {
        dog->SetSpeed({0, map_speed});
        dog->SetDirection(model::Direction::SOUTH);
    } else {
        return false;
    }

    return true;
}

void ProcessTickUseCase::ProcessTick(std::int64_t tick) {
    game_->UpdateState(tick);
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

bool Application::MoveDog(std::string_view token, std::string_view move) {
    return manage_dog_actions_use_case_.MoveDog(token, move);
}

void Application::ProcessTick(std::int64_t tick) {
    process_tick_use_case_.ProcessTick(tick);
}

bool Application::IsTokenValid(std::string_view token) const {
    if (tokens_.FindPlayerByToken(user::Token{std::string(token)}) == nullptr) {
        return false;
    }
    return true;
}

} // namespace app
