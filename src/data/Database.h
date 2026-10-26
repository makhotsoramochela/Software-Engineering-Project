// Layer 3 (data access): the connection to SQLite, prepared statements and transactions.
// AR-3: SQL is written only in this layer, and every value goes in a ? placeholder.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Ports.h"

struct sqlite3;
struct sqlite3_stmt;

namespace lms {

// One prepared statement. It is finalised when the object is destroyed.
// Parameter and column numbers: parameters start at 1, columns start at 0 (as in SQLite).
class Statement {
public:
    Statement(sqlite3* db, sqlite3_stmt* stmt);
    ~Statement();
    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    void bindInt(int index, std::int64_t value);
    void bindDouble(int index, double value);
    void bindText(int index, const std::string& value);
    void bindBlob(int index, const std::vector<std::uint8_t>& value);
    void bindNull(int index);
    void bindOptionalInt(int index, const std::optional<int>& value);
    void bindOptionalDouble(int index, const std::optional<double>& value);
    void bindOptionalText(int index, const std::optional<std::string>& value);

    // Returns true if there is a row, false when the statement is done. Throws DatabaseError on an error.
    bool step();

    std::int64_t columnInt(int index) const;
    double columnDouble(int index) const;
    std::string columnText(int index) const;
    std::vector<std::uint8_t> columnBlob(int index) const;
    bool columnIsNull(int index) const;
    std::optional<double> columnOptionalDouble(int index) const;
    std::optional<std::string> columnOptionalText(int index) const;
    std::optional<int> columnOptionalInt(int index) const;

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_;
};

class Database : public UnitOfWork {
public:
    // Opens (or creates) the file. A new file gets the schema. Use ":memory:" for tests.
    explicit Database(const std::string& path);
    ~Database() override;
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    std::unique_ptr<Transaction> begin() override;
    Statement prepare(const std::string& sql);
    void exec(const std::string& sql);
    std::int64_t lastInsertId() const;
    sqlite3* handle() { return handle_; }

    static constexpr int SCHEMA_VERSION = 1;

private:
    void ensureSchema();
    sqlite3* handle_ = nullptr;
};

class SqliteTransaction : public Transaction {
public:
    explicit SqliteTransaction(Database& db);
    ~SqliteTransaction() override;  // rolls back if commit() was not called
    void commit() override;

private:
    Database& db_;
    bool finished_ = false;
};

}  // namespace lms
