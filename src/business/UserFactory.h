// Layer 2 (business logic): Simple Factory pattern. One place chooses the subclass of User
// for a role (design: Figure 3.8 and Table 3.5).
#pragma once

#include <memory>
#include <optional>
#include <string>

#include "User.h"

namespace lms {

class UserFactory {
public:
    // departmentId is required for a Head of Department and must be empty for the other roles.
    std::unique_ptr<User> create(Role role, int id, const std::string& username, const std::string& fullName,
                                 const std::string& passwordHash, std::optional<int> departmentId) const;
};

}  // namespace lms
