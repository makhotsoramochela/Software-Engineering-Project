// Layer 2 (business logic): the abstract User class and its four roles (design: Figure 3.1).
// Polymorphism: each subclass says which role it is and which functions it owns (Table 10).
#pragma once

#include <optional>
#include <string>

#include "DateTime.h"
#include "Types.h"

namespace lms {

class User {
public:
    User(int id, std::string username, std::string fullName, std::string passwordHash, bool active = true,
         int failedLogins = 0, std::optional<DateTime> lockedUntil = std::nullopt);
    virtual ~User() = default;

    int getId() const { return id_; }
    const std::string& getUsername() const { return username_; }
    const std::string& getFullName() const { return fullName_; }
    const std::string& getPasswordHash() const { return passwordHash_; }
    bool isActive() const { return active_; }
    int getFailedLogins() const { return failedLogins_; }
    const std::optional<DateTime>& getLockedUntil() const { return lockedUntil_; }

    void setFullName(std::string name) { fullName_ = std::move(name); }
    void setPasswordHash(std::string hash) { passwordHash_ = std::move(hash); }
    void setActive(bool active) { active_ = active; }
    void setFailedLogins(int count) { failedLogins_ = count; }
    void setLockedUntil(std::optional<DateTime> until) { lockedUntil_ = until; }

    bool isLocked(const DateTime& now) const;
    bool can(Function function) const;

    virtual Role role() const = 0;
    virtual FunctionSet permissions() const = 0;

    // Only a Head of Department has a department. For the other roles departmentId() is empty and
    // setDepartmentId() refuses. This way no code needs to ask "what kind of user is this?".
    virtual std::optional<int> departmentId() const { return std::nullopt; }
    virtual void setDepartmentId(int departmentId);

private:
    int id_;
    std::string username_;
    std::string fullName_;
    std::string passwordHash_;
    bool active_;
    int failedLogins_;
    std::optional<DateTime> lockedUntil_;
};

class Administrator : public User {
public:
    using User::User;
    Role role() const override { return Role::ADMINISTRATOR; }
    FunctionSet permissions() const override;
};

class Lecturer : public User {
public:
    using User::User;
    Role role() const override { return Role::LECTURER; }
    FunctionSet permissions() const override;
};

class HeadOfDepartment : public User {
public:
    HeadOfDepartment(int id, std::string username, std::string fullName, std::string passwordHash, int departmentId,
                     bool active = true, int failedLogins = 0, std::optional<DateTime> lockedUntil = std::nullopt);
    int getDepartmentId() const { return departmentId_; }
    std::optional<int> departmentId() const override { return departmentId_; }
    void setDepartmentId(int departmentId) override { departmentId_ = departmentId; }
    Role role() const override { return Role::HEAD_OF_DEPARTMENT; }
    FunctionSet permissions() const override;

private:
    int departmentId_;
};

class Student : public User {
public:
    using User::User;
    Role role() const override { return Role::STUDENT; }
    FunctionSet permissions() const override;
};

}  // namespace lms
