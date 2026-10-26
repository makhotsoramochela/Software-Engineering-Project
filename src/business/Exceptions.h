// Layer 2 (business logic): the family of exceptions (design: Figure 3.9, rule AR-6).
#pragma once

#include <stdexcept>
#include <string>

namespace lms {

// Base class of every error that the program raises on purpose.
class LmsError : public std::runtime_error {
public:
    explicit LmsError(const std::string& message) : std::runtime_error(message) {}
};

// Input is empty, too long, or in the wrong form.
class ValidationError : public LmsError {
public:
    explicit ValidationError(const std::string& message) : LmsError(message) {}
};

// The role or the data of the user does not allow the action.
class PermissionError : public LmsError {
public:
    explicit PermissionError(const std::string& message) : LmsError(message) {}
};

// A record does not exist.
class NotFoundError : public LmsError {
public:
    explicit NotFoundError(const std::string& message) : LmsError(message) {}
};

// A business rule forbids the action.
class RuleError : public LmsError {
public:
    explicit RuleError(const std::string& message) : LmsError(message) {}
};

// SQLite reported an error.
class DatabaseError : public LmsError {
public:
    explicit DatabaseError(const std::string& message) : LmsError(message) {}
};

// Wrong password, unknown user or locked account.
class AuthenticationError : public LmsError {
public:
    explicit AuthenticationError(const std::string& message) : LmsError(message) {}
};

}  // namespace lms
