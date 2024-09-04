#include <sstream>

#include "player.h"

namespace user {

const model::Dog* Player::GetDog() const {
    return dog_;
}

const model::GameSession* Player::GetGameSession() const {
    return session_;
}

Token PlayerTokens::GenerateUniqueToken() {
    Token token{""};
    do {
        std::ostringstream str;
        str << std::hex << generator1_() << generator2_();
        *token = str.str();
    } while (token_to_player_.contains(token) && (*token).size() != 32);
    return token;
}

Token PlayerTokens::AddPlayer(Player* player) {
    auto token = GenerateUniqueToken();
    token_to_player_[token] = player;
    return token;
}

Player* PlayerTokens::FindPlayerByToken(const Token& token) {
    if (token_to_player_.contains(token)) {
        return token_to_player_.at(token);
    }
    return nullptr;
}

const Player* PlayerTokens::FindPlayerByToken(const Token& token) const {
    if (token_to_player_.contains(token)) {
        return token_to_player_.at(token);
    }
    return nullptr;
}

Player& Players::Add(model::Dog* dog, model::GameSession* session) {
    players_.push_back(std::make_unique<Player>(session, dog));
    Player* player = players_.back().get();
    map_to_dog_to_player_[session->GetMapId()][dog->GetId()] = player;
    return *player;
}

Player* Players::FindByDogIdAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) {
    if (map_to_dog_to_player_.contains(map_id) && map_to_dog_to_player_[map_id].contains(dog_id)) {
        return map_to_dog_to_player_[map_id][dog_id];
    }
    return nullptr;
}

} // namespace user
