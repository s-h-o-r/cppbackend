#include "retirement_detector.h"

#include <vector>

namespace retirement {
void RetirementListener::DogMove(model::Dog* dog, std::int64_t tick, bool move) {
    dog_retirement_.at(dog).time_in_game += tick;
    if (move) {
        dog_retirement_.at(dog).no_action_time = 0;
    } else {
        dog_retirement_.at(dog).no_action_time += tick;
    }
}

void RetirementListener::OnTick(std::chrono::milliseconds delta) {
    std::vector<model::Dog*> dog_for_retirement;

    for (auto& [dog, statistic] : dog_retirement_) {
        if (statistic.no_action_time >= retirement_time_) {
            dog_for_retirement.push_back(dog);
            continue;
        }
    }

    for (model::Dog* dog : dog_for_retirement) {
        app_->SaveToLeaderboard(dog->GetName(), dog->GetScore(), dog_retirement_.at(dog).time_in_game);

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
