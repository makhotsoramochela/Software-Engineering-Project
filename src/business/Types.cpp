#include "Types.h"

#include "Exceptions.h"

namespace lms {

std::string toString(Role role) {
    switch (role) {
        case Role::ADMINISTRATOR:
            return "ADMINISTRATOR";
        case Role::LECTURER:
            return "LECTURER";
        case Role::HEAD_OF_DEPARTMENT:
            return "HEAD_OF_DEPARTMENT";
        case Role::STUDENT:
            return "STUDENT";
    }
    throw RuleError("Unknown role");
}

Role parseRole(const std::string& text) {
    if (text == "ADMINISTRATOR") return Role::ADMINISTRATOR;
    if (text == "LECTURER") return Role::LECTURER;
    if (text == "HEAD_OF_DEPARTMENT") return Role::HEAD_OF_DEPARTMENT;
    if (text == "STUDENT") return Role::STUDENT;
    throw DatabaseError("Unknown role in the database: " + text);
}

std::string toString(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::SUBMITTED:
            return "SUBMITTED";
        case SubmissionStatus::LATE:
            return "LATE";
        case SubmissionStatus::MARKED:
            return "MARKED";
        case SubmissionStatus::RETURNED:
            return "RETURNED";
        case SubmissionStatus::MODERATED:
            return "MODERATED";
        case SubmissionStatus::RELEASED:
            return "RELEASED";
    }
    throw RuleError("Unknown status");
}

SubmissionStatus parseStatus(const std::string& text) {
    if (text == "SUBMITTED") return SubmissionStatus::SUBMITTED;
    if (text == "LATE") return SubmissionStatus::LATE;
    if (text == "MARKED") return SubmissionStatus::MARKED;
    if (text == "RETURNED") return SubmissionStatus::RETURNED;
    if (text == "MODERATED") return SubmissionStatus::MODERATED;
    if (text == "RELEASED") return SubmissionStatus::RELEASED;
    throw DatabaseError("Unknown status in the database: " + text);
}

std::string displayName(Role role) {
    switch (role) {
        case Role::ADMINISTRATOR:
            return "Administrator";
        case Role::LECTURER:
            return "Lecturer";
        case Role::HEAD_OF_DEPARTMENT:
            return "Head of Department";
        case Role::STUDENT:
            return "Student";
    }
    return "Unknown";
}

std::string displayName(SubmissionStatus status) {
    switch (status) {
        case SubmissionStatus::SUBMITTED:
            return "Submitted";
        case SubmissionStatus::LATE:
            return "Late";
        case SubmissionStatus::MARKED:
            return "Marked";
        case SubmissionStatus::RETURNED:
            return "Returned";
        case SubmissionStatus::MODERATED:
            return "Moderated";
        case SubmissionStatus::RELEASED:
            return "Released";
    }
    return "Unknown";
}

}  // namespace lms
