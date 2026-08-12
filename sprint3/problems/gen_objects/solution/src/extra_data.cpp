#include "extra_data.h"

namespace extra_data {

void ExtraData::AddLootTypes(const Map& map, const boost::json::array& loot_types) {
    loot_types_.emplace(map.GetId(), loot_types);
}

const boost::json::array& ExtraData::GetLootTypes(const Map& map) const {
    return loot_types_.at(map.GetId());
}

} // namespace extra_data