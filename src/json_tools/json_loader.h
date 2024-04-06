#pragma once

#include <filesystem>
#include <fstream>
#include <exception>

#include <boost/json.hpp>

#include "../model/model.h"
#include "../extra/extra_data.h"

namespace json_loader {
	const std::string default_dog_speed_string = "defaultDogSpeed";
	const std::string default_bag_capacity_string = "defaulBagCapacity";
	const std::string dog_retirement_time_string = "dogRetirementTime";
	const std::string bag_capacity_string = "BagCapacity";
	const std::string loot_types_string = "lootTypes";
	const std::string dog_speed_string = "dogSpeed";
	const std::string maps_string = "maps";
	const std::string id_string = "id";
	const std::string name_string = "name";
	const std::string x_string = "x";
	const std::string y_string = "y";
	

	class JsonError : public std::exception
	{
	public:
		JsonError(const std::string& message) : message{ message } {
		}
		const char* what() const noexcept override {
			return message.c_str();
		}
	private:
		std::string message;
	};

	namespace json = boost::json;

	model::Map MakeMap(const json::object& json_map);

	void AddRoads(const json::object& json_map, model::Map& map);

	void AddBuildings(const json::object& json_map, model::Map& map);

	void AddOffices(const json::object& json_map, model::Map& map);

	void SetDogSpeed(const json::object& json_map, model::Map& map);

	model::Game LoadGame(std::filesystem::path json_path);

	extra_data::Json_data LoadExtraData(std::filesystem::path json_path);

}   // namespace json_loader
