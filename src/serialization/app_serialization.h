#pragma once


#include <fstream>
#include <iostream>

#include "../app/app.h"
#include "../model/model.h"
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/vector.hpp>



namespace app_serialization {

    struct GameState{
	   std::string map_id;
	   std::vector<model::Dog> dogs;
	   std::vector<app::Token> tokens;
	   std::map<size_t, model::LostObject> lost_objects;

	   template <typename Archive>
	   void serialize(Archive& ar, [[maybe_unused]] const unsigned int file_version) {
		   ar& map_id;
		   ar& dogs;
		   ar& tokens;
		   ar& lost_objects;
	   }

    };


	class SerializingListener : public app::ApplicationListener
	{
	public:
		SerializingListener(app::Players& players, model::Game& game, app::PlayerTokens& tokens):
			app::ApplicationListener(players, game, tokens){}
		void OnTick(double time_delta) {
			if (is_save_ && is_auto_save_) {
				time_since_save_ += time_delta;
				if (time_since_save_ >= save_interval_) {
				   Serialize();
				   time_since_save_ = 0.;
				}
			}
		}

		void Serialize(); 

		void Deserialize();

		void SetSaveInterval(double save_interval) {
			save_interval_ = save_interval;
		}

		void SetParams(double is_save, double is_auto_save, double save_interval, std::filesystem::path state_file_path) {
			is_save_ = is_save;
			is_auto_save_ = is_auto_save;
			save_interval_ = save_interval;
			state_file_path_ = state_file_path;
		}

	private:
		double save_interval_ = 5000.;
		double time_since_save_ = 0.;

		bool is_save_ = false;
		bool is_auto_save_ = false;



		std::filesystem::path state_file_path_;

	};
}//namespace app_serialization


