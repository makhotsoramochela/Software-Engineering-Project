// Layer 2 (business logic): input validation and permission checks used by the services.
// Invalid input is stopped here with a ValidationError (NFR-08), before it reaches the data layer.
#pragma once

#include <string>

#include "Exceptions.h"
#include "User.h"

namespace lms {

// AR-4: the first line of every service method. Throws PermissionError if the role does not own the function.
void requirePermission(const User& actor, Function function);

// The department of a Head of Department. Throws PermissionError for any other role.
int departmentOf(const User& actor);

namespace validate {

// Removes spaces at both ends. Throws ValidationError if the result is empty or longer than maxLength.
std::string text(const std::string& value, const std::string& fieldName, std::size_t maxLength);

// Letters, digits, dot, dash and underscore only; 3 to 50 characters.
std::string username(const std::string& value);

// BR-08: at least 8 characters, with at least one letter and one digit.
void password(const std::string& value);

// BR-09: a mark from 0 to the maximum mark, with at most one decimal place.
void mark(double value, int maxMark);

// BR-09: a maximum mark is a whole number from 1 to 100.
void maxMark(int value);

}  // namespace validate

}  // namespace lms
