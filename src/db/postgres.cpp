#include "postgres.h"

namespace postgres {

using namespace std::literals;

namespace {

constexpr auto CREATE_TABLE = R"(
CREATE TABLE IF NOT EXISTS retired_players (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    score INT NOT NULL,
    play_time DOUBLE PRECISION NOT NULL
);
)"sv;

constexpr auto CREATE_INDEX = R"(
CREATE INDEX IF NOT EXISTS idx_retired_players_leaderboard
ON retired_players (score DESC, play_time ASC, name ASC);
)"sv;

constexpr auto INSERT_RECORD =
    "INSERT INTO retired_players (name, score, play_time) VALUES ($1, $2, $3);"sv;

constexpr auto SELECT_RECORDS =
    "SELECT name, score, play_time FROM retired_players "
    "ORDER BY score DESC, play_time ASC, name ASC "
    "OFFSET $1 LIMIT $2;"sv;

}  // namespace

void LeaderboardRepository::SaveRetiredPlayer(const model::PlayerRecord& record) {
    auto conn = pool_.GetConnection();
    pqxx::work tx{*conn};
    tx.exec_params(std::string{INSERT_RECORD}, record.name, record.score, record.play_time);
    tx.commit();
}

std::vector<LeaderboardEntry> LeaderboardRepository::GetRecords(size_t offset, size_t limit) {
    auto conn = pool_.GetConnection();
    pqxx::read_transaction tx{*conn};

    const auto result = tx.exec_params(std::string{SELECT_RECORDS},
                                       static_cast<long long>(offset),
                                       static_cast<long long>(limit));

    std::vector<LeaderboardEntry> entries;
    entries.reserve(result.size());
    for (const auto& row : result) {
        entries.push_back({row[0].as<std::string>(), row[1].as<int>(), row[2].as<double>()});
    }
    return entries;
}

Database::Database(const std::string& db_url, size_t capacity)
    : pool_{capacity, [db_url] { return std::make_shared<pqxx::connection>(db_url); }}
    , leaderboard_{pool_} {
    auto conn = pool_.GetConnection();
    pqxx::work tx{*conn};
    tx.exec(std::string{CREATE_TABLE});
    tx.exec(std::string{CREATE_INDEX});
    tx.commit();
}

}  // namespace postgres
