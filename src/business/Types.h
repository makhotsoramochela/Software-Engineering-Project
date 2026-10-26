// Layer 2 (business logic): enumerations of the domain model (design: Figure 3.1 and 3.3).
#pragma once

#include <set>
#include <string>

namespace lms {

enum class Role { ADMINISTRATOR, LECTURER, HEAD_OF_DEPARTMENT, STUDENT };

// The functions of Table 10 of the SRS. A role owns a set of them.
enum class Function {
    MANAGE_USERS,
    MANAGE_COURSES,
    ASSIGN_LECTURERS,
    ENROL_STUDENTS,
    CREATE_ASSIGNMENT,
    GRANT_EXTENSION,
    MARK_SUBMISSION,
    MODERATE_MARKS,
    RELEASE_MARKS,
    VIEW_AUDIT_LOG,
    VIEW_COURSES,
    SUBMIT_WORK,
    VIEW_RELEASED_MARKS
};

using FunctionSet = std::set<Function>;

enum class SubmissionStatus { SUBMITTED, LATE, MARKED, RETURNED, MODERATED, RELEASED };

// The text forms are the ones stored in the database (CHECK constraints of the schema).
std::string toString(Role role);
Role parseRole(const std::string& text);
std::string toString(SubmissionStatus status);
SubmissionStatus parseStatus(const std::string& text);

// A friendly name for screens, for example "Head of Department".
std::string displayName(Role role);
std::string displayName(SubmissionStatus status);

}  // namespace lms
