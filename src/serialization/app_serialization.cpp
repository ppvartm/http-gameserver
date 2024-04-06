#include "app_serialization.h"


namespace app_serialization {

	void SerializingListener::Serialize() {

		if (game_.GetGameSessions().size() != 0) {
			std::vector<GameState> game_states;
			for (int i = 0; i < game_.GetGameSessions().size(); ++i) {
				std::string map_id = *(*game_.GetGameSessions()[i]).GetMapId();

				std::vector<app::Token> tokens;
				std::vector<model::Dog> dogs;

				for (auto p : game_.GetGameSessions()[i]->GetDogs())
					dogs.push_back(*(p.second));

				for (auto d : dogs)
					for (auto p : tokens_.GetTokens())
						if (p.second->GetDog()->GetId() == d.GetId())
							tokens.push_back(p.first);

				auto lost_object = (*game_.GetGameSessions()[i]).GetCurrentLostObjects();

				game_states.push_back({ map_id, dogs, tokens, lost_object });
			}

			std::stringstream ss_;
			boost::archive::text_oarchive oa{ ss_ };
			oa << game_states;

			std::ofstream file;

			file.open(state_file_path_);
			file << ss_.str();
			file.close();

		}
	}

	void SerializingListener::Deserialize() {
		std::vector<GameState> game_states;

		GameState game_state;
		std::stringstream ss_;
		std::ifstream file;
		file.open(state_file_path_);
		ss_ << file.rdbuf();
		boost::archive::text_iarchive ia{ ss_ };
		ia >> game_states;
		file.close();
		for (int i = 0; i < game_states.size(); ++i) {
			auto gs = game_.FindGameSession(model::Map::Id(game_states[i].map_id));
			for (int j = 0; j < game_states[i].dogs.size(); ++j) {
				tokens_.AddPlayer(game_states[i].tokens[j], players_.Add(std::make_shared<model::Dog>(game_states[i].dogs[j]), gs, "Regain"));
			}
			gs->GetCurrentLostObjects() = game_states[i].lost_objects;
		}
	}
}
