#include "Validation.h"

#include <cctype>
#include <cmath>

namespace lms {

void requirePermission(const User& actor, Function function) {
    if (!actor.can(function)) {
        throw PermissionError("Your role is not allowed to do this.");
    }
}

int departmentOf(const User& actor) {
    const std::optional<int> department = actor.departmentId();
    if (!department) throw PermissionError("Only a Head of Department has a department.");
    return *department;
}

namespace validate {

std::string text(const std::string& value, const std::string& fieldName, std::size_t maxLength) {
    std::size_t first = 0;
    std::size_t last = value.size();
    while (first < last && std::isspace(static_cast<unsigned char>(value[first]))) ++first;
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) --last;
    const std::string trimmed = value.substr(first, last - first);
    if (trimmed.empty()) throw ValidationError(fieldName + " must not be empty.");
    if (trimmed.size() > maxLength) {
        throw ValidationError(fieldName + " must have at most " + std::to_string(maxLength) + " characters.");
    }
    return trimmed;
}

std::string username(const std::string& value) {
    const std::string name = text(value, "Username", 50);
    if (name.size() < 3) throw ValidationError("Username must have at least 3 characters.");
    for (const char c : name) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (!(std::isalnum(u) || c == '.' || c == '-' || c == '_')) {
            throw ValidationError("Username may have only letters, digits, dot, dash and underscore.");
        }
    }
    return name;
}

void password(const std::string& value) {
    bool letter = false;
    bool digit = false;
    for (const char c : value) {
        const unsigned char u = static_cast<unsigned char>(c);
        if (std::isalpha(u)) letter = true;
        if (std::isdigit(u)) digit = true;
    }
    if (value.size() < 8 || !letter || !digit) {
        throw ValidationError("A password needs at least 8 characters, with at least one letter and one digit.");
    }
    if (value.size() > 128) throw ValidationError("A password may have at most 128 characters.");
}

void mark(double value, int maxMark) {
    if (std::isnan(value) || value < 0 || value > maxMark) {
        throw ValidationError("A mark must be from 0 to " + std::to_string(maxMark) + ".");
    }
    const double tenths = value * 10.0;
    if (std::fabs(tenths - std::round(tenths)) > 1e-9) {
        throw ValidationError("A mark may have at most one decimal place.");
    }
}

void maxMark(int value) {
    if (value < 1 || value > 100) throw ValidationError("The maximum mark must be a whole number from 1 to 100.");
}

}  // namespace validate

}  // namespace lms
