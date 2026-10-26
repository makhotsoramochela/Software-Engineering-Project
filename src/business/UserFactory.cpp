#include "UserFactory.h"

#include "Exceptions.h"

namespace lms {

std::unique_ptr<User> UserFactory::create(Role role, int id, const std::string& username, const std::string& fullName,
                                          const std::string& passwordHash, std::optional<int> departmentId) const {
    if (role == Role::HEAD_OF_DEPARTMENT) {
        if (!departmentId) throw ValidationError("A Head of Department must have a department.");
        return std::make_unique<HeadOfDepartment>(id, username, fullName, passwordHash, *departmentId);
    }
    if (departmentId) throw ValidationError("Only a Head of Department has a department.");
    switch (role) {
        case Role::ADMINISTRATOR:
            return std::make_unique<Administrator>(id, username, fullName, passwordHash);
        case Role::LECTURER:
            return std::make_unique<Lecturer>(id, username, fullName, passwordHash);
        case Role::STUDENT:
            return std::make_unique<Student>(id, username, fullName, passwordHash);
        case Role::HEAD_OF_DEPARTMENT:
            break;
    }
    throw ValidationError("Unknown role.");
}

}  // namespace lms
