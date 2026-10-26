// A complete system on an in-memory database with a fake clock and a fast fake password hasher.
// The tests use the real services and the real SQLite repositories.
#pragma once

#include <memory>

#include "AdminService.h"
#include "AssignmentService.h"
#include "AuditService.h"
#include "AuthService.h"
#include "Exceptions.h"
#include "Database.h"
#include "MarkingService.h"
#include "MessageService.h"
#include "ModerationService.h"
#include "PenaltyStrategy.h"
#include "SqliteRepositories.h"
#include "SubmissionService.h"

namespace test {

class FakeClock : public lms::Clock {
public:
    lms::DateTime now() const override { return now_; }
    void set(const std::string& text) { now_ = lms::DateTime::parse(text); }
    void advanceMinutes(long long m) { now_ = now_.plusMinutes(m); }

private:
    lms::DateTime now_ = lms::DateTime::parse("2026-11-01 09:00");
};

// Not secure. It makes the tests fast. The real hasher has its own test.
class FakeHasher : public lms::PasswordHasher {
public:
    std::string hash(const std::string& password) const override { return "fake$" + password; }
    bool verify(const std::string& hash, const std::string& password) const override {
        return hash == "fake$" + password;
    }
};

struct World {
    lms::Database db{":memory:"};
    FakeClock clock;
    FakeHasher hasher;
    lms::TenPercentPerDayPenalty penalty;
    lms::SqliteUserRepository users{db};
    lms::SqliteCourseRepository courses{db};
    lms::SqliteAssignmentRepository assignments{db};
    lms::SqliteSubmissionRepository submissions{db};
    lms::SqliteAuditRepository audits{db};
    lms::SqliteMessageRepository messages{db};
    lms::AuthService auth{users, hasher, clock, db};
    lms::AdminService admin{users, courses, hasher, db};
    lms::MessageService messageService{messages, clock};
    lms::AuditService audit{audits, clock};
    lms::AssignmentService assignmentService{assignments, courses, submissions, users, penalty, clock, db};
    lms::SubmissionService submissionService{submissions, assignments, courses, assignmentService, penalty, clock, db};
    lms::MarkingService marking{submissions, assignments, courses, penalty, audit, clock, db};
    lms::ModerationService moderation{submissions, assignments, courses, audit, messageService, clock, db};

    std::unique_ptr<lms::User> adminUser, lecturer, otherLecturer, hod, otherHod, student1, student2, student3;
    int csDepartment = 0, mathsDepartment = 0, course = 0, mathsCourse = 0, assignment = 0;

    // Deadline of the assignment: 2026-11-10 12:00. Maximum mark: 50. The clock starts at 2026-11-01 09:00.
    World() {
        using lms::Role;
        admin.createFirstAdministrator("admin", "The Administrator", "Admin1234");
        adminUser = users.findByUsername("admin");
        csDepartment = admin.createDepartment(*adminUser, "Computer Science");
        mathsDepartment = admin.createDepartment(*adminUser, "Mathematics");
        course = admin.createCourse(*adminUser, "CS4433", "Software Engineering", csDepartment);
        mathsCourse = admin.createCourse(*adminUser, "MA2101", "Calculus", mathsDepartment);
        make("lecturer1", "Lecturer One", Role::LECTURER, std::nullopt, lecturer);
        make("lecturer2", "Lecturer Two", Role::LECTURER, std::nullopt, otherLecturer);
        make("hod_cs", "Head of CS", Role::HEAD_OF_DEPARTMENT, csDepartment, hod);
        make("hod_ma", "Head of Maths", Role::HEAD_OF_DEPARTMENT, mathsDepartment, otherHod);
        make("student1", "Student One", Role::STUDENT, std::nullopt, student1);
        make("student2", "Student Two", Role::STUDENT, std::nullopt, student2);
        make("student3", "Student Three", Role::STUDENT, std::nullopt, student3);
        admin.assignLecturer(*adminUser, course, lecturer->getId());
        admin.assignLecturer(*adminUser, mathsCourse, otherLecturer->getId());
        for (auto* s : {student1.get(), student2.get(), student3.get()})
            admin.enrolStudent(*adminUser, course, s->getId());
        assignment = assignmentService.createAssignment(*lecturer, course, "Assignment 1", "Do the work.",
                                                        lms::DateTime::parse("2026-11-10 12:00"), 50);
    }

    static std::vector<std::uint8_t> file(std::size_t size = 100) { return std::vector<std::uint8_t>(size, 0x41); }

    // Submits a file for the student at the time of the fake clock.
    lms::Receipt submit(const lms::User& student, const std::string& name = "work.txt") {
        const auto preview = submissionService.prepareSubmission(student, assignment, name, file());
        return submissionService.confirmSubmission(student, preview);
    }

    int submissionIdOf(const lms::User& student) { return submissions.find(assignment, student.getId())->getId(); }

private:
    void make(const std::string& username, const std::string& name, lms::Role role, std::optional<int> department,
              std::unique_ptr<lms::User>& out) {
        admin.createUser(*adminUser, username, name, "Password1", role, department);
        out = users.findByUsername(username);
    }
};

}  // namespace test
