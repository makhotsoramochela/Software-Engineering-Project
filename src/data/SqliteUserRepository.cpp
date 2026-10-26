#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

namespace {
// Column order used by every SELECT of this file.
const char* const USER_COLUMNS =
    "user_id, username, full_name, password_hash, role, department_id, active, failed_logins, locked_until";
}  // namespace

std::unique_ptr<User> SqliteUserRepository::fromRow(const Statement& row) const {
    const Role role = parseRole(row.columnText(4));
    std::optional<int> departmentId = row.columnOptionalInt(5);
    std::unique_ptr<User> user = factory_.create(role, static_cast<int>(row.columnInt(0)), row.columnText(1),
                                                 row.columnText(2), row.columnText(3), departmentId);
    user->setActive(row.columnInt(6) != 0);
    user->setFailedLogins(static_cast<int>(row.columnInt(7)));
    if (!row.columnIsNull(8)) user->setLockedUntil(DateTime::parse(row.columnText(8)));
    return user;
}

std::unique_ptr<User> SqliteUserRepository::findById(int id) {
    Statement st = db_.prepare(std::string("SELECT ") + USER_COLUMNS + " FROM user_account WHERE user_id = ?");
    st.bindInt(1, id);
    return st.step() ? fromRow(st) : nullptr;
}

std::unique_ptr<User> SqliteUserRepository::findByUsername(const std::string& username) {
    Statement st = db_.prepare(std::string("SELECT ") + USER_COLUMNS + " FROM user_account WHERE username = ?");
    st.bindText(1, username);
    return st.step() ? fromRow(st) : nullptr;
}

std::unique_ptr<User> SqliteUserRepository::activeHeadOf(int departmentId) {
    Statement st =
        db_.prepare(std::string("SELECT ") + USER_COLUMNS +
                    " FROM user_account WHERE role = 'HEAD_OF_DEPARTMENT' AND department_id = ? AND active = 1");
    st.bindInt(1, departmentId);
    return st.step() ? fromRow(st) : nullptr;
}

std::vector<std::unique_ptr<User>> SqliteUserRepository::list() {
    Statement st = db_.prepare(std::string("SELECT ") + USER_COLUMNS + " FROM user_account ORDER BY user_id");
    std::vector<std::unique_ptr<User>> users;
    while (st.step()) users.push_back(fromRow(st));
    return users;
}

int SqliteUserRepository::add(const User& user) {
    const std::optional<int> departmentId = user.departmentId();
    Statement st = db_.prepare(
        "INSERT INTO user_account (username, full_name, password_hash, role, department_id, active, failed_logins, "
        "locked_until) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    st.bindText(1, user.getUsername());
    st.bindText(2, user.getFullName());
    st.bindText(3, user.getPasswordHash());
    st.bindText(4, toString(user.role()));
    st.bindOptionalInt(5, departmentId);
    st.bindInt(6, user.isActive() ? 1 : 0);
    st.bindInt(7, user.getFailedLogins());
    const std::optional<DateTime> locked = user.getLockedUntil();
    st.bindOptionalText(8, locked ? std::optional<std::string>(locked->toString()) : std::nullopt);
    st.step();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteUserRepository::update(const User& user) {
    const std::optional<int> departmentId = user.departmentId();
    Statement st = db_.prepare(
        "UPDATE user_account SET full_name = ?, password_hash = ?, department_id = ?, active = ?, "
        "failed_logins = ?, locked_until = ? WHERE user_id = ?");
    st.bindText(1, user.getFullName());
    st.bindText(2, user.getPasswordHash());
    st.bindOptionalInt(3, departmentId);
    st.bindInt(4, user.isActive() ? 1 : 0);
    st.bindInt(5, user.getFailedLogins());
    const std::optional<DateTime> locked = user.getLockedUntil();
    st.bindOptionalText(6, locked ? std::optional<std::string>(locked->toString()) : std::nullopt);
    st.bindInt(7, user.getId());
    st.step();
}

}  // namespace lms
