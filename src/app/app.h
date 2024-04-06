#pragma once
#include <memory>
#include <random>
#include <unordered_map>
#include <iostream>

#include <boost/functional/hash.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../model/model.h"
#include "../database_tools/postgres.h"

namespace app {
    using Token = std::string;

    //move the dog; associated with certain dog and certain game-session
	class Player {

	public:
        Player() {}

        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session):
            dog_(dog), 
            game_session_(game_session){
            game_session->AddDog(dog_);
        }

        Player(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session, std::string) :
            dog_(dog),
            game_session_(game_session) {
            game_session->RegainDog(dog_);
        }

        model::Dog::Id GetDogId() {
            return dog_->GetId();
        }

        std::shared_ptr<model::Dog> GetDog() const {
            return dog_;
        }

        const std::shared_ptr<model::GameSession> GetGameSession() const {
            return game_session_;
        }

        void SetDogSpeed(model::DogSpeedFromJson speed_horizontal, model::DogSpeedFromJson speed_vertical) {
            dog_->SetSpeed({speed_horizontal, speed_vertical});
        }

        void SetDogDirection(const std::string& dir) {
            dog_->SetDirection(dir);
        }

        std::vector<model::Road> GetRoadsWithDog();

        model::Position NewCorrectPosition(model::Position new_position);

        void MoveDog(double time_delta);

        void CheckRetirementTime(double time_delta);

        double GetDownTime() {
            return dog_->GetDownTime();
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
            ar& *dog_;
        }

        void SetRetirementTime(double retirement_time) {
            retirement_time_ = retirement_time;
        }

        double GetRetirementTime() {
            return retirement_time_;
        }

        double GetPlayTime() {
            return dog_->GetPlayTime();
        }
        
        void IncreasePlayTime(double time_delta) {
            dog_->IncreacePlayTime(time_delta);
        }
	private:
        std::shared_ptr<model::Dog> dog_;
		std::shared_ptr<model::GameSession> game_session_;

        double retirement_time_ = 0;
	};

    //contains the pairs player-tocken 
    class PlayerTokens {  
    public:
        Token AddPlayer(std::shared_ptr<Player> player) {
            Token token = GenerateToken();
            token_to_player_[token] = player;
            return token;
        }

        void AddPlayer(Token token, std::shared_ptr<Player> player) {
            token_to_player_[token] = player;
        }

        std::shared_ptr<Player> FindPlayerByToken(const Token& token) {
            auto player = token_to_player_.find(token);
            if (player != token_to_player_.end())
                return player->second;
            return nullptr;
        }

        std::unordered_map<Token, std::shared_ptr<Player>> GetTokens() const {
            return token_to_player_;
        }

        std::vector<model::Dog::Id> CheckRetirementTime() {
            std::vector<model::Dog::Id> list_of_id_for_dog_deletion;
            std::vector<Token> tokens_for_deletion;
            for (auto& p : token_to_player_)
                if (p.second->GetDownTime() >= p.second->GetRetirementTime())
                    tokens_for_deletion.push_back(p.first);
            for (auto& p : tokens_for_deletion) {
                list_of_id_for_dog_deletion.push_back(token_to_player_[p]->GetDogId());
                token_to_player_.erase(p);
            }
            return list_of_id_for_dog_deletion;
        }

        template <typename Archive>
        void serialize(Archive& ar, [[maybe_unused]] const unsigned int version) {
            ar& token_to_player_;
        }

    private:
        std::unordered_map<Token, std::shared_ptr<Player>> token_to_player_;
        std::random_device random_device_;
        std::mt19937_64 generator1_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        std::mt19937_64 generator2_{ [this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }() };
        Token GenerateToken();
    };

    //contains the all players
    class Players {
    public:
        std::shared_ptr<Player> Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session) {
            return players_[{dog->GetId(), *(game_session->GetMapId())}] = std::make_shared<Player>(dog, game_session);
        }   

        std::shared_ptr<Player> Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<model::GameSession> game_session, std::string str) {
            return players_[{dog->GetId(), * (game_session->GetMapId())}] = std::make_shared<Player>(dog, game_session, str);
        }

        std::shared_ptr<Player> FindByDogAndMapId(model::Dog::Id dog_id, model::Map::Id map_id) {
            return players_[{dog_id, *map_id}];
        }

        void MoveAllDogs(double time_delta) {
            for (auto& player: players_)
                player.second->MoveDog(time_delta);
        }

        void IncreaseAllTimes(double time_delta) {
            for (auto& player : players_)
                player.second->IncreasePlayTime(time_delta);
        }

    private:
        std::unordered_map<std::pair<uint64_t, std::string>, std::shared_ptr<Player>, boost::hash<std::pair<uint64_t, std::string>>> players_;
    };

    //to manage the game clock
    class ApplicationListener {
    public:
        ApplicationListener(Players& players, model::Game& game, PlayerTokens& tokens) :
            players_(players), game_(game), tokens_(tokens) {
        }

        virtual void OnTick(double time_delta) = 0;
    protected:
        Players& players_;
        model::Game& game_;
        PlayerTokens& tokens_;
    };

    class GameTimer {
    public:

        GameTimer(Players& players, PlayerTokens& player_tokens, const model::Game::GameSessions& game_sessions,
                  boost::asio::strand<boost::asio::io_context::executor_type>& strand, ApplicationListener& app_listener, postgres_tools::PostgresDatabase& database) :
            players_(players), tokens_(player_tokens), game_sessions_(game_sessions), strand_(strand), app_listener_(app_listener), database_(database) {}

        void Tick(std::chrono::milliseconds time_delta);
    private:
        Players& players_;
        PlayerTokens& tokens_;
        const model::Game::GameSessions& game_sessions_;


        boost::asio::strand<boost::asio::io_context::executor_type>& strand_;

        ApplicationListener& app_listener_;
        postgres_tools::PostgresDatabase& database_;
    };
    
 
}