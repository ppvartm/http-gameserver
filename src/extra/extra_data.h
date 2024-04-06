#pragma once

#include <boost/json.hpp>




namespace extra_data{

	class Json_data {
	public:

		void Push(const boost::json::array& data, const std::string& map_id) {
			lootTypes_for_all_Maps_[map_id] = data;
		}

		const boost::json::array& Get(const std::string& map_id) const {
			return lootTypes_for_all_Maps_.at(map_id);
		}

	private:

		std::map<std::string, boost::json::array> lootTypes_for_all_Maps_;
	};

}