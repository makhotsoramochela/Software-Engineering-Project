#include "Database.h"

#include <sqlite3.h>

#include <utility>

#include "Exceptions.h"

namespace lms {

extern const char* const SCHEMA_SQL;  // generated from sql/schema.sql by CMake

namespace {
[[noreturn]] void fail(sqlite3* db, const std::string& what) {
    throw DatabaseError(what + ": " + (db != nullptr ? sqlite3_errmsg(db) : "no database"));
}
}  // namespace

// Statement

Statement::Statement(sqlite3* db, sqlite3_stmt* stmt) : db_(db), stmt_(stmt) {}

Statement::~Statement() {
    if (stmt_ != nullptr) sqlite3_finalize(stmt_);
}

Statement::Statement(Statement&& other) noexcept : db_(other.db_), stmt_(other.stmt_) {
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_ != nullptr) sqlite3_finalize(stmt_);
        db_ = other.db_;
        stmt_ = other.stmt_;
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::bindInt(int index, std::int64_t value) {
    if (sqlite3_bind_int64(stmt_, index, value) != SQLITE_OK) fail(db_, "Cannot bind a number");
}

void Statement::bindDouble(int index, double value) {
    if (sqlite3_bind_double(stmt_, index, value) != SQLITE_OK) fail(db_, "Cannot bind a number");
}

void Statement::bindText(int index, const std::string& value) {
    if (sqlite3_bind_text(stmt_, index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        fail(db_, "Cannot bind a text");
    }
}

void Statement::bindBlob(int index, const std::vector<std::uint8_t>& value) {
    // A zero-length blob is bound as an empty blob (not NULL); the schema rejects it by its CHECK.
    const void* data = value.empty() ? "" : static_cast<const void*>(value.data());
    if (sqlite3_bind_blob(stmt_, index, data, static_cast<int>(value.size()), SQLITE_TRANSIENT) != SQLITE_OK) {
        fail(db_, "Cannot bind a file");
    }
}

void Statement::bindNull(int index) {
    if (sqlite3_bind_null(stmt_, index) != SQLITE_OK) fail(db_, "Cannot bind NULL");
}

void Statement::bindOptionalInt(int index, const std::optional<int>& value) {
    value ? bindInt(index, *value) : bindNull(index);
}

void Statement::bindOptionalDouble(int index, const std::optional<double>& value) {
    value ? bindDouble(index, *value) : bindNull(index);
}

void Statement::bindOptionalText(int index, const std::optional<std::string>& value) {
    value ? bindText(index, *value) : bindNull(index);
}

bool Statement::step() {
    const int code = sqlite3_step(stmt_);
    if (code == SQLITE_ROW) return true;
    if (code == SQLITE_DONE) return false;
    fail(db_, "The database could not do the action");
}

std::int64_t Statement::columnInt(int index) const {
    return sqlite3_column_int64(stmt_, index);
}

double Statement::columnDouble(int index) const {
    return sqlite3_column_double(stmt_, index);
}

std::string Statement::columnText(int index) const {
    const unsigned char* text = sqlite3_column_text(stmt_, index);
    if (text == nullptr) return {};
    return std::string(reinterpret_cast<const char*>(text),
                       static_cast<std::size_t>(sqlite3_column_bytes(stmt_, index)));
}

std::vector<std::uint8_t> Statement::columnBlob(int index) const {
    const void* data = sqlite3_column_blob(stmt_, index);
    const int size = sqlite3_column_bytes(stmt_, index);
    if (data == nullptr || size <= 0) return {};
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    return std::vector<std::uint8_t>(bytes, bytes + size);
}

bool Statement::columnIsNull(int index) const {
    return sqlite3_column_type(stmt_, index) == SQLITE_NULL;
}

std::optional<double> Statement::columnOptionalDouble(int index) const {
    return columnIsNull(index) ? std::nullopt : std::optional<double>(columnDouble(index));
}

std::optional<std::string> Statement::columnOptionalText(int index) const {
    return columnIsNull(index) ? std::nullopt : std::optional<std::string>(columnText(index));
}

std::optional<int> Statement::columnOptionalInt(int index) const {
    return columnIsNull(index) ? std::nullopt : std::optional<int>(static_cast<int>(columnInt(index)));
}

// Database

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &handle_) != SQLITE_OK) {
        const std::string message = handle_ != nullptr ? sqlite3_errmsg(handle_) : "out of memory";
        if (handle_ != nullptr) sqlite3_close(handle_);
        handle_ = nullptr;
        throw DatabaseError("Cannot open the database file " + path + ": " + message);
    }
    try {
        sqlite3_busy_timeout(handle_, 5000);
        exec("PRAGMA foreign_keys = ON;");  // on every new connection (see the schema)
        ensureSchema();
    } catch (...) {
        sqlite3_close(handle_);
        handle_ = nullptr;
        throw;
    }
}

Database::~Database() {
    if (handle_ != nullptr) sqlite3_close(handle_);
}

void Database::ensureSchema() {
    Statement check = prepare("SELECT COUNT(*) FROM sqlite_master WHERE type = 'table' AND name = 'user_account'");
    check.step();
    const bool exists = check.columnInt(0) > 0;
    if (!exists) {
        SqliteTransaction transaction(*this);  // a new file gets the whole schema or nothing
        exec(SCHEMA_SQL);
        exec("PRAGMA user_version = " + std::to_string(SCHEMA_VERSION) + ";");
        transaction.commit();
        return;
    }
    Statement version = prepare("PRAGMA user_version");
    version.step();
    if (version.columnInt(0) != SCHEMA_VERSION) {
        throw DatabaseError("The database file has an unknown version. This program needs version " +
                            std::to_string(SCHEMA_VERSION) + ".");
    }
}

std::unique_ptr<Transaction> Database::begin() {
    return std::make_unique<SqliteTransaction>(*this);
}

Statement Database::prepare(const std::string& sql) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(handle_, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) != SQLITE_OK) {
        fail(handle_, "Cannot prepare a statement");
    }
    return Statement(handle_, stmt);
}

void Database::exec(const std::string& sql) {
    char* error = nullptr;
    if (sqlite3_exec(handle_, sql.c_str(), nullptr, nullptr, &error) != SQLITE_OK) {
        const std::string message = error != nullptr ? error : "unknown error";
        sqlite3_free(error);
        throw DatabaseError("The database could not do the action: " + message);
    }
}

std::int64_t Database::lastInsertId() const {
    return sqlite3_last_insert_rowid(handle_);
}

//SqliteTransaction

SqliteTransaction::SqliteTransaction(Database& db) : db_(db) {
    db_.exec("BEGIN IMMEDIATE;");
}

SqliteTransaction::~SqliteTransaction() {
    if (!finished_) {
        char* error = nullptr;
        sqlite3_exec(db_.handle(), "ROLLBACK;", nullptr, nullptr, &error);  // never throws from a destructor
        sqlite3_free(error);
    }
}

void SqliteTransaction::commit() {
    db_.exec("COMMIT;");
    finished_ = true;
}

}  // namespace lms
