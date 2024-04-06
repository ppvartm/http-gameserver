#pragma once
#include <pqxx/pqxx>
#include <string>

#include <boost/json.hpp>

#include "../model/model.h"

namespace postgres_tools {

	using pqxx::operator"" _zv;


	struct Record {
		std::string name;
		int score;
		double play_time;
	};

	class PostgresDatabase {
	public:
		PostgresDatabase(const std::string& conn);

		void AddRecord(const std::string& name, int score, double play_time);

		std::vector<Record> GetRecords();

		void AddRecordsAllDogs(model::GameSession& game_session, std::vector<model::Dog::Id> list_id_for_deletion);

	private:
		pqxx::connection conn_;		
	};

	void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Record& record);

}//namespace postgres_tools