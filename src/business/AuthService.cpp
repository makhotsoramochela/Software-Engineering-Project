#include "AuthService.h"

#include "Exceptions.h"

namespace lms {

AuthService::AuthService(IUserRepository& users, PasswordHasher& hasher, Clock& clock, UnitOfWork& uow)
    : users_(users), hasher_(hasher), clock_(clock), uow_(uow) {}

std::unique_ptr<User> AuthService::login(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        throw ValidationError("Enter both the username and the password.");
    }
    std::unique_ptr<User> user = users_.findByUsername(username);
    if (!user || !user->isActive()) {
        // The same work and the same message as for a wrong password, so the answer does not
        // tell a stranger which usernames exist.
        hasher_.hash(password);
        throw AuthenticationError("Wrong username or password.");
    }
    const DateTime now = clock_.now();
    const std::optional<DateTime> lockedUntil = user->getLockedUntil();
    if (lockedUntil && now < *lockedUntil) {
        throw AuthenticationError("This account is locked until " + lockedUntil->toDisplay() + ".");
    }
    if (!hasher_.verify(user->getPasswordHash(), password)) {
        handleWrongPassword(*user, now);  // always throws
    }
    if (user->getFailedLogins() != 0 || user->getLockedUntil().has_value()) {
        user->setFailedLogins(0);
        user->setLockedUntil(std::nullopt);
        saveUser(*user);
    }
    return user;
}

// Counts the wrong password, locks the account after MAX_WRONG of them, and throws the error.
void AuthService::handleWrongPassword(User& user, const DateTime& now) {
    const int wrong = user.getFailedLogins() + 1;
    const bool mustLock = (wrong >= MAX_WRONG);
    if (mustLock) {
        user.setFailedLogins(0);  // the count starts again when the lock ends
        user.setLockedUntil(now.plusMinutes(LOCK_MINUTES));
    } else {
        user.setFailedLogins(wrong);
    }
    saveUser(user);  // saved before the error is thrown
    if (mustLock) {
        throw AuthenticationError("Too many wrong passwords. The account is locked for " +
                                  std::to_string(LOCK_MINUTES) + " minutes.");
    }
    throw AuthenticationError("Wrong username or password.");
}

void AuthService::saveUser(const User& user) {
    auto transaction = uow_.begin();
    users_.update(user);
    transaction->commit();
}

}  // namespace lms
