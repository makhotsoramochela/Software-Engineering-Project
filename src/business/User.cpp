#include "User.h"

#include "Exceptions.h"

namespace lms {

void User::setDepartmentId(int /*departmentId*/) {
    throw ValidationError("Only a Head of Department has a department.");
}

User::User(int id, std::string username, std::string fullName, std::string passwordHash, bool active, int failedLogins,
           std::optional<DateTime> lockedUntil)
    : id_(id),
      username_(std::move(username)),
      fullName_(std::move(fullName)),
      passwordHash_(std::move(passwordHash)),
      active_(active),
      failedLogins_(failedLogins),
      lockedUntil_(lockedUntil) {}

bool User::isLocked(const DateTime& now) const {
    return lockedUntil_.has_value() && now < *lockedUntil_;
}

bool User::can(Function function) const {
    return permissions().count(function) > 0;
}

// Table 10 of the SRS. "Own courses", "own department" and "own work" are checked by the services.
FunctionSet Administrator::permissions() const {
    return {Function::MANAGE_USERS, Function::MANAGE_COURSES, Function::ASSIGN_LECTURERS, Function::ENROL_STUDENTS};
}

FunctionSet Lecturer::permissions() const {
    return {Function::VIEW_COURSES, Function::CREATE_ASSIGNMENT, Function::GRANT_EXTENSION, Function::MARK_SUBMISSION};
}

HeadOfDepartment::HeadOfDepartment(int id, std::string username, std::string fullName, std::string passwordHash,
                                   int departmentId, bool active, int failedLogins, std::optional<DateTime> lockedUntil)
    : User(id, std::move(username), std::move(fullName), std::move(passwordHash), active, failedLogins, lockedUntil),
      departmentId_(departmentId) {}

FunctionSet HeadOfDepartment::permissions() const {
    return {Function::MODERATE_MARKS, Function::RELEASE_MARKS, Function::VIEW_AUDIT_LOG};
}

FunctionSet Student::permissions() const {
    return {Function::VIEW_COURSES, Function::SUBMIT_WORK, Function::VIEW_RELEASED_MARKS};
}

}  // namespace lms
