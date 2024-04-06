#include "app.h"

namespace app {
    //Player
    std::vector<model::Road> Player::GetRoadsWithDog() {
        double width_of_road = model::WIDTH_OF_ROAD;
        std::vector<model::Road> valid_roads;
        for (auto& road : game_session_->GetMap().GetRoads()) {
            if (road.IsHorizontal()) {
                if ((dog_->GetPosition().y >= road.GetStart().y - width_of_road && dog_->GetPosition().y <= road.GetEnd().y + width_of_road) &&
                    ((dog_->GetPosition().x >= road.GetStart().x - width_of_road && dog_->GetPosition().x <= road.GetEnd().x + width_of_road) ||
                        (dog_->GetPosition().x <= road.GetStart().x + width_of_road && dog_->GetPosition().x >= road.GetEnd().x - width_of_road)))
                    valid_roads.push_back(road);
            }
            else if (road.IsVertical()) {
                if ((dog_->GetPosition().x >= road.GetStart().x - width_of_road && dog_->GetPosition().x <= road.GetEnd().x + width_of_road) &&
                    ((dog_->GetPosition().y >= road.GetStart().y - width_of_road && dog_->GetPosition().y <= road.GetEnd().y + width_of_road) ||
                        (dog_->GetPosition().y <= road.GetStart().y + width_of_road && dog_->GetPosition().y >= road.GetEnd().y - width_of_road)))
                    valid_roads.push_back(road);
            }
        }
        return valid_roads;

    }

    model::Position Player::NewCorrectPosition(model::Position new_position) {
        auto roads = GetRoadsWithDog();
        auto answer = new_position;
        double width_of_road = model::WIDTH_OF_ROAD;
        for (auto& road : roads) {
            if (road.IsHorizontal()) {
                if (new_position.y > road.GetStart().y + width_of_road)
                    answer.y = road.GetStart().y + width_of_road;
                if (new_position.y < road.GetStart().y - width_of_road)
                    answer.y = road.GetStart().y - width_of_road;

                if (road.GetStart().x < road.GetEnd().x) {
                    if (new_position.x < road.GetStart().x - width_of_road)
                        answer.x = road.GetStart().x - width_of_road;
                    if (new_position.x > road.GetEnd().x + width_of_road)
                        answer.x = road.GetEnd().x + width_of_road;
                }
                else {
                    if (new_position.x > road.GetStart().x + width_of_road)
                        answer.x = road.GetStart().x + width_of_road;
                    if (new_position.x < road.GetEnd().x - width_of_road)
                        answer.x = road.GetEnd().x - width_of_road;
                }
            }
            else {
                if (new_position.x > road.GetStart().x + width_of_road)
                    answer.x = road.GetStart().x + width_of_road;
                if (new_position.x < road.GetStart().x - width_of_road)
                    answer.x = road.GetStart().x - width_of_road;

                if (road.GetStart().y < road.GetEnd().y) {
                    if (new_position.y < road.GetStart().y - width_of_road)
                        answer.y = road.GetStart().y - width_of_road;
                    if (new_position.y > road.GetEnd().y + width_of_road)
                        answer.y = road.GetEnd().y + width_of_road;
                }
                else {
                    if (new_position.y > road.GetStart().y + width_of_road)
                        answer.y = road.GetStart().y + width_of_road;
                    if (new_position.y < road.GetEnd().y - width_of_road)
                        answer.y = road.GetEnd().y - width_of_road;
                }
            }
            if (roads.size() == 1)
                return answer;
            if (answer.x == new_position.x && answer.y == new_position.y)
                return answer;
            if (road.IsHorizontal() && (dog_->GetDirection() == model::Direction::WEST || dog_->GetDirection() == model::Direction::EAST))
                return answer;
            if (road.IsVertical() && (dog_->GetDirection() == model::Direction::SOUTH || dog_->GetDirection() == model::Direction::NORTH))
                return answer;
            answer = new_position;
        }
        return answer;
    }

    void Player::CheckRetirementTime(double time_delta) {
        if (dog_->GetSpeed().s_x == 0. && dog_->GetSpeed().s_y == 0.)
            dog_->IncreaceDownTime(time_delta);
        if (dog_->GetSpeed().s_x != 0. || dog_->GetSpeed().s_y != 0.)
            dog_->ResetDownTime();
    }

    void Player::MoveDog(double time_delta) {  //время в миллисикундах
       
        if (dog_->GetSpeed().s_x == 0. && dog_->GetSpeed().s_y == 0.)
            dog_->IncreaceDownTime(time_delta / 1000);
        if (dog_->GetSpeed().s_x != 0. || dog_->GetSpeed().s_y != 0.)
            dog_->ResetDownTime();

        model::Position new_position = { 
            dog_->GetPosition().x + dog_->GetSpeed().s_x * time_delta / CLOCKS_PER_SEC,
            dog_->GetPosition().y + dog_->GetSpeed().s_y * time_delta / CLOCKS_PER_SEC
        };
        model::Position new_correct_position = NewCorrectPosition(new_position);
        if ((new_position.x != new_correct_position.x) || (new_position.y != new_correct_position.y))
            SetDogSpeed(0., 0.);
        dog_->SetPosition(new_correct_position);



    }

	Token PlayerTokens::GenerateToken() {
        std::stringstream ss;
        ss << std::hex << generator1_();
        ss << std::hex << generator2_();
        while (ss.str().size() < 32)
            ss << rand() % 9;
        return ss.str();
	}

    void GameTimer::Tick(std::chrono::milliseconds time_delta) {
        boost::asio::dispatch(strand_, [this, time_delta]() {
            players_.MoveAllDogs(std::chrono::duration<double>(time_delta).count() * CLOCKS_PER_SEC);
            players_.IncreaseAllTimes(std::chrono::duration<double>(time_delta).count());
            auto list_id_for_deletion = tokens_.CheckRetirementTime();

            for (int i = 0; i < game_sessions_.size(); ++i)
                database_.AddRecordsAllDogs(*game_sessions_[i], list_id_for_deletion);

            for (int i = 0; i < game_sessions_.size(); ++i)
                game_sessions_[i]->DeleteDogs(list_id_for_deletion);

            for (int i = 0; i < game_sessions_.size(); ++i)
                game_sessions_[i]->GenerateLoot(time_delta);

            for (int i = 0; i < game_sessions_.size(); ++i) {
                game_sessions_[i]->CollectionItems(std::chrono::duration<double>(time_delta).count() * CLOCKS_PER_SEC);
                game_sessions_[i]->LeaveItems(std::chrono::duration<double>(time_delta).count() * CLOCKS_PER_SEC);
            }
            app_listener_.OnTick(std::chrono::duration<double>(time_delta).count() * CLOCKS_PER_SEC);

        });
    }
}