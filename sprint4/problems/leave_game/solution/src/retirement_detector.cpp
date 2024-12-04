#include "retirement_detector.h"

#include <vector>

namespace retirement {
void RetirementListener::DogMove(model::Dog* dog, std::int64_t tick, bool move) {
    if (move) {
        dog_retirement_.at(dog).no_action_time = 0;
    } else {
        dog_retirement_.at(dog).no_action_time += tick;
    }
}

void RetirementListener::OnTick(std::chrono::milliseconds delta) {
    std::vector<model::Dog*> dog_for_retirement;
    std::vector<detail::LeaderboardInfo> to_database;

    for (auto& [dog, statistic] : dog_retirement_) {
        statistic.time_in_game += delta.count();
        if (statistic.no_action_time >= retirement_time_) {
            dog_for_retirement.push_back(dog);
            to_database.push_back({dog->GetName(), dog->GetScore(), statistic.time_in_game});
            continue;
        }
    }

    for (auto& info : to_database) {
        app_->SaveToLeaderboard(info.name, info.score, info.time_in_game_ms);
    }

    for (model::Dog* dog : dog_for_retirement) {
        user::Token token{dog_to_token_.at(dog)};
        dog_retirement_.erase(dog);
        dog_to_token_.erase(dog);
        app_->DeletePlayer(*token);
    }
}

void RetirementListener::OnJoin(std::string token, model::Dog* dog) {
    dog_retirement_[dog] = {0, 0};
    dog_to_token_[dog] = std::move(token);
}

}
