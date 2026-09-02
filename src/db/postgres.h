#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "connection_pool.h"
#include "../model/player_record.h"

namespace postgres {

// A leaderboard row is just a persisted PlayerRecord.
using LeaderboardEntry = model::PlayerRecord;

class LeaderboardRepository {
public:
    explicit LeaderboardRepository(ConnectionPool& pool)
        : pool_{pool} {
    }

    void SaveRetiredPlayer(const model::PlayerRecord& record);

    [[nodiscard]] std::vector<LeaderboardEntry> GetRecords(size_t offset, size_t limit);

private:
    ConnectionPool& pool_;
};

class Database {
public:
    Database(const std::string& db_url, size_t capacity);

    LeaderboardRepository& GetLeaderboard() noexcept {
        return leaderboard_;
    }

private:
    ConnectionPool pool_;
    LeaderboardRepository leaderboard_;
};

}  // namespace postgres
