#include "model.h"

#include <stdexcept>

namespace model {
using namespace std::literals;

void Map::AddOffice(const Office& office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(office);
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        offices_.pop_back();
        throw;
    }
}

void GameSession::RegainDog(std::shared_ptr<Dog> dog) {
    dogs_.insert({ dog->GetName(),dog });
}

void GameSession::AddDog(std::shared_ptr<Dog> dog) {
    srand(time(0));
    Position pos;
    if (!is_rand_spawn_) {
        pos = { static_cast<double>(map_.GetRoads().begin()->GetStart().x),static_cast<double>(map_.GetRoads().begin()->GetStart().y) };
    }
    else {
        auto road = map_.GetRoads()[random_int() % map_.GetRoads().size()];
        if (road.IsVertical() && road.GetEnd().y > road.GetStart().y) {
            pos.x = road.GetStart().x;
            pos.y = (random_int() % (road.GetEnd().y - road.GetStart().y)) + road.GetStart().y;
        }
        if (road.IsVertical() && road.GetEnd().y < road.GetStart().y) {
            pos.x = road.GetStart().x;
            pos.y = (random_int() % (road.GetStart().y - road.GetEnd().y)) + road.GetEnd().y;
        }
        if (road.IsHorizontal() && road.GetEnd().x > road.GetStart().x) {
            pos.y = road.GetStart().y;
            pos.x = (random_int() % (road.GetEnd().x - road.GetStart().x)) + road.GetStart().x;
        }
        if (road.IsHorizontal() && road.GetEnd().x < road.GetStart().x) {
            pos.y = road.GetStart().y;
            pos.x = (random_int() % (road.GetStart().x - road.GetEnd().x)) + road.GetEnd().x;
        }
    }
    dog->SetPosition(pos);
    dog->SetBagCapacity(map_.GetBagCapacity());
    dogs_.insert({ dog->GetName(),dog });
}

void GameSession::GenerateLoot(std::chrono::milliseconds time_delta) {
    unsigned need_to_gen = loot_generator_.Generate(time_delta, GetCurrentLostObjects().size(), dogs_.size());
    srand(time(NULL));
    for (unsigned i = 0; i < need_to_gen; ++i) {
        int type = random_int() % map_.GetMaxCountOfLootObjects();
        int value = map_.GetPriceList().find(type)->second;
        Position pos;
        auto road = map_.GetRoads()[random_int() % map_.GetRoads().size()];
        if (road.IsVertical() && road.GetEnd().y > road.GetStart().y) {
            pos.x = road.GetStart().x;
            pos.y = (random_int() % (road.GetEnd().y - road.GetStart().y)) + road.GetStart().y;
        }
        if (road.IsVertical() && road.GetEnd().y < road.GetStart().y) {
            pos.x = road.GetStart().x;
            pos.y = (random_int() % (road.GetStart().y - road.GetEnd().y)) + road.GetEnd().y;
        }
        if (road.IsHorizontal() && road.GetEnd().x > road.GetStart().x) {
            pos.y = road.GetStart().y;
            pos.x = (random_int() % (road.GetEnd().x - road.GetStart().x)) + road.GetStart().x;
        }
        if (road.IsHorizontal() && road.GetEnd().x < road.GetStart().x) {
            pos.y = road.GetStart().y;
            pos.x = (random_int() % (road.GetStart().x - road.GetEnd().x)) + road.GetEnd().x;
        }
        PushLostObject({ type, pos, value });
    }
}

void GameSession::GenerateForced() {
    int type = random_int() % map_.GetMaxCountOfLootObjects();
    Position pos;
    int value = map_.GetPriceList().find(type)->second;
    auto road = map_.GetRoads()[random_int() % map_.GetRoads().size()];
    if (road.IsVertical() && road.GetEnd().y > road.GetStart().y) {
        pos.x = road.GetStart().x;
        pos.y = (random_int() % (road.GetEnd().y - road.GetStart().y)) + road.GetStart().y;
    }
    if (road.IsVertical() && road.GetEnd().y < road.GetStart().y) {
        pos.x = road.GetStart().x;
        pos.y = (random_int() % (road.GetStart().y - road.GetEnd().y)) + road.GetEnd().y;
    }
    if (road.IsHorizontal() && road.GetEnd().x > road.GetStart().x) {
        pos.y = road.GetStart().y;
        pos.x = (random_int() % (road.GetEnd().x - road.GetStart().x)) + road.GetStart().x;
    }
    if (road.IsHorizontal() && road.GetEnd().x < road.GetStart().x) {
        pos.y = road.GetStart().y;
        pos.x = (random_int() % (road.GetStart().x - road.GetEnd().x)) + road.GetEnd().x;
    }
    PushLostObject({ type, pos, value });
}

Dog& GameSession::GetDog(size_t id) const {
    auto dog = dogs_.begin();
    for (int i = 0; i < id; ++i)
        ++dog;
    return *(dog->second);
}

std::pair<size_t, LostObject> GameSession::GetLostObject(size_t id) const {
    auto lost_object = lost_objects_.begin();
    for (int i = 0; i < id; ++i)
        ++lost_object;
    return *lost_object;
}

void GameSession::CollectionItems(double time_delta) {
    std::vector<GatheringEvent> result;
    auto lost_objects = lost_objects_;
    for (size_t i = 0; i < lost_objects.size(); ++i) {
        auto item = GetLostObject(i);
        for (size_t g = 0; g < dogs_.size(); ++g) {
            auto gatherer = GetDog(g);
            auto collection_result = collision_detector::TryCollectPoint(
                { gatherer.GetPosition().x, gatherer.GetPosition().y },
                { gatherer.GetNextPosition(time_delta).x,gatherer.GetNextPosition(time_delta).y },
                { item.second.position.x, item.second.position.y });
            if (collection_result.IsCollected(WIDTH_OF_DOG)) {
                if (!gatherer.IsFullBag())
                    result.push_back({ g ,item.first, collection_result.proj_ratio });
            }
        }
        if (!result.empty()) {
            std::sort(result.begin(), result.end(), [](const GatheringEvent& e_l, const GatheringEvent& e_r) {
                return e_l.time < e_r.time;
                });
            GetDog(result.begin()->dog_id).AddObjectInBag(item.first, item.second);
            lost_objects_.erase(item.first);
            result.clear();
        }
    }

}

void GameSession::LeaveItems(double time_delta) {
    for (size_t g = 0; g < dogs_.size(); ++g) {
        auto gatherer = GetDog(g);
        for (size_t i = 0; i < map_.GetOffices().size(); ++i) {
            auto collection_result = collision_detector::TryCollectPoint(
                { gatherer.GetPosition().x, gatherer.GetPosition().y },
                { gatherer.GetNextPosition(time_delta).x,gatherer.GetNextPosition(time_delta).y },
                { static_cast<double>(map_.GetOffices()[i].GetPosition().x), static_cast<double>(map_.GetOffices()[i].GetPosition().y) });
            if (collection_result.IsCollected(WIDTH_OF_DOG + WIDTH_OF_OFFICE)) {
                gatherer.AddScore();
                gatherer.RemoveObjectsFromBag();
            }
        }
    }
}

void Game::AddMap(const Map& map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            maps_.emplace_back(map);
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Road& road)
{

    if (road.IsHorizontal())
        jv = {
            {"x0", road.GetStart().x},
            {"y0", road.GetStart().y},
            {"x1", road.GetEnd().x}
    };
    if (road.IsVertical())
        jv = {
            {"x0", road.GetStart().x},
            {"y0", road.GetStart().y},
            {"y1", road.GetEnd().y}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Building& build) {
    jv = {
        {"x", build.GetBounds().position.x},
        {"y", build.GetBounds().position.y},
        {"w", build.GetBounds().size.width},
        {"h", build.GetBounds().size.height}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Office& office) {
    jv = {
        {"id", *office.GetId()},
        {"x", office.GetPosition().x},
        {"y", office.GetPosition().y},
        {"offsetX", office.GetOffset().dx},
        {"offsetY", static_cast<int>(office.GetOffset().dy)}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const Map& map) {
    jv = {
        {"id", *map.GetId()},
        {"name", map.GetName()},
        {"roads", map.GetRoads()},
        {"buildings", map.GetBuildings()},
        {"offices", map.GetOffices()}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, Game const& game) {
    jv = {
        {"maps", game.GetMaps()}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const std::pair<Map, boost::json::array>& map_and_json_data) {
    jv = {
            {"id", *map_and_json_data.first.GetId()},
            {"name", map_and_json_data.first.GetName()},
            {"roads", map_and_json_data.first.GetRoads()},
            {"buildings", map_and_json_data.first.GetBuildings()},
            {"offices", map_and_json_data.first.GetOffices()},
            {"lootTypes", map_and_json_data.second}
    };
}

void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, const std::pair<size_t, LostObject>& lost_object) {
    jv = {
        {"id", lost_object.first},
        {"type", lost_object.second.type}
    };
}

int random_int() {
    return rand();
}

}  // namespace model
