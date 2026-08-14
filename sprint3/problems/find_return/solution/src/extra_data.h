#pragma once
#include <boost/json.hpp>
#include "model/model.h"

namespace extra_data {

using namespace model;

class ExtraData {
public:
    void AddLootTypes(const Map& map, const boost::json::array& loot_types);
    const boost::json::array& GetLootTypes(const Map& map) const;

private:
    std::unordered_map<Map::Id, boost::json::array, util::TaggedHasher<Map::Id>> loot_types_;
};

} // namespace extra_data