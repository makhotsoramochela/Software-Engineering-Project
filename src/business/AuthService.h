// Layer 2 (business logic): UC-01 Log in, with the lock of FR-03.
#pragma once

#include <memory>
#include <string>

#include "Ports.h"

namespace lms {

class AuthService {
public:
    AuthService(IUserRepository& users, PasswordHasher& hasher, Clock& clock, UnitOfWork& uow);

    // Returns the user if the username and password are right and the account is active and not locked.
    // After MAX_WRONG wrong passwords in a row the account is locked for LOCK_MINUTES (FR-03).
    std::unique_ptr<User> login(const std::string& username, const std::string& password);

private:
    void handleWrongPassword(User& user, const DateTime& now);
    void saveUser(const User& user);

    IUserRepository& users_;
    PasswordHasher& hasher_;
    Clock& clock_;
    UnitOfWork& uow_;
    static constexpr int MAX_WRONG = 5;
    static constexpr int LOCK_MINUTES = 5;
};

}  // namespace lms
