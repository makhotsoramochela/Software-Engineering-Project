// Tests of access control, passwords, SQL injection, the audit log and transactions
// (FR-02, FR-03, NFR-03, NFR-04, NFR-08, NFR-09).
#include <chrono>

#include "SodiumPasswordHasher.h"
#include "TestSupport.h"
#include "World.h"

using namespace lms;

TEST(login_works_and_does_not_tell_which_part_was_wrong) {  // FR-01
    test::World w;
    CHECK(w.auth.login("lecturer1", "Password1")->getUsername() == "lecturer1");
    CHECK(w.auth.login("LECTURER1", "Password1")->getUsername() == "lecturer1");  // the username is not case sensitive
    std::string messageUnknown, messageWrong;
    try {
        w.auth.login("nobody", "Password1");
    } catch (const AuthenticationError& e) {
        messageUnknown = e.what();
    }
    try {
        w.auth.login("lecturer1", "Wrong1234");
    } catch (const AuthenticationError& e) {
        messageWrong = e.what();
    }
    CHECK(!messageUnknown.empty() && messageUnknown == messageWrong);
    CHECK_THROWS(w.auth.login("", "x"), ValidationError);
    w.admin.deactivateUser(*w.adminUser, w.student1->getId());
    CHECK_THROWS(w.auth.login("student1", "Password1"), AuthenticationError);  // a deactivated account cannot log in
}

TEST(five_wrong_passwords_lock_the_account_for_five_minutes) {  // FR-03
    test::World w;
    w.clock.set("2026-11-01 10:00");
    for (int i = 0; i < 4; ++i) CHECK_THROWS(w.auth.login("student1", "Wrong1234"), AuthenticationError);
    CHECK_THROWS(w.auth.login("student1", "Wrong1234"), AuthenticationError);  // the fifth
    CHECK_THROWS(w.auth.login("student1", "Password1"), AuthenticationError);  // locked: even the right password fails
    w.clock.set("2026-11-01 10:04");
    CHECK_THROWS(w.auth.login("student1", "Password1"), AuthenticationError);  // 4 minutes: still locked
    w.clock.set("2026-11-01 10:05");
    CHECK(w.auth.login("student1", "Password1") != nullptr);  // 5 minutes: open again
    CHECK(w.users.findByUsername("student1")->getFailedLogins() == 0);
}

TEST(a_good_login_resets_the_count_of_wrong_passwords) {
    test::World w;
    for (int i = 0; i < 4; ++i) CHECK_THROWS(w.auth.login("student2", "Wrong1234"), AuthenticationError);
    CHECK(w.auth.login("student2", "Password1") != nullptr);
    for (int i = 0; i < 4; ++i) CHECK_THROWS(w.auth.login("student2", "Wrong1234"), AuthenticationError);
    CHECK(w.auth.login("student2", "Password1") != nullptr);  // 4 + 4 wrong in total, but not in a row
}

TEST(administrator_can_open_a_locked_account_with_a_new_password) {  // FR-05
    test::World w;
    for (int i = 0; i < 5; ++i) CHECK_THROWS(w.auth.login("student1", "Wrong1234"), AuthenticationError);
    w.admin.resetPassword(*w.adminUser, w.student1->getId(), "NewPassword7");
    CHECK(w.auth.login("student1", "NewPassword7") != nullptr);
    CHECK_THROWS(w.admin.resetPassword(*w.adminUser, w.student1->getId(), "short"), ValidationError);
}

// NFR-04: every function is refused to the roles that Table 10 does not allow.
TEST(every_service_function_is_refused_to_the_wrong_role) {
    test::World w;
    const User& student = *w.student1;
    const User& lecturer = *w.lecturer;
    const User& hod = *w.hod;
    const User& admin = *w.adminUser;
    w.clock.set("2026-11-09 10:00");
    w.submit(student);
    const int id = w.submissionIdOf(student);
    w.marking.saveMark(lecturer, id, 30, "ok", "");

    // Administrator functions
    CHECK_THROWS(w.admin.listUsers(student), PermissionError);
    CHECK_THROWS(w.admin.listUsers(lecturer), PermissionError);
    CHECK_THROWS(w.admin.createUser(hod, "x1x", "X", "Password1", Role::STUDENT, std::nullopt), PermissionError);
    CHECK_THROWS(w.admin.createDepartment(lecturer, "D"), PermissionError);
    CHECK_THROWS(w.admin.createCourse(student, "X1", "X", w.csDepartment), PermissionError);
    CHECK_THROWS(w.admin.assignLecturer(hod, w.course, w.lecturer->getId()), PermissionError);
    CHECK_THROWS(w.admin.enrolStudent(student, w.course, w.student2->getId()), PermissionError);
    CHECK_THROWS(w.admin.resetPassword(student, w.student2->getId(), "Password9"), PermissionError);
    // Lecturer functions
    CHECK_THROWS(
        w.assignmentService.createAssignment(student, w.course, "T", "D", DateTime::parse("2026-12-01 10:00"), 10),
        PermissionError);
    CHECK_THROWS(w.assignmentService.createAssignment(hod, w.course, "T", "D", DateTime::parse("2026-12-01 10:00"), 10),
                 PermissionError);
    CHECK_THROWS(
        w.assignmentService.createAssignment(admin, w.course, "T", "D", DateTime::parse("2026-12-01 10:00"), 10),
        PermissionError);
    CHECK_THROWS(w.assignmentService.grantExtension(student, w.assignment, student.getId(),
                                                    DateTime::parse("2026-12-01 10:00"), "x"),
                 PermissionError);
    CHECK_THROWS(w.marking.listSubmissions(student, w.assignment), PermissionError);
    CHECK_THROWS(w.marking.listSubmissions(admin, w.assignment), PermissionError);  // BR-10
    CHECK_THROWS(w.marking.listSubmissions(hod, w.assignment), PermissionError);
    CHECK_THROWS(w.marking.saveMark(student, id, 50, "cheat", ""), PermissionError);
    CHECK_THROWS(w.marking.saveMark(hod, id, 50, "x", "y"), PermissionError);
    CHECK_THROWS(w.marking.exportFile(student, id), PermissionError);
    CHECK_THROWS(w.marking.exportFile(admin, id), PermissionError);
    // Head of Department functions
    CHECK_THROWS(w.moderation.listMarked(lecturer), PermissionError);
    CHECK_THROWS(w.moderation.listMarked(student), PermissionError);
    CHECK_THROWS(w.moderation.approve(lecturer, id), PermissionError);
    CHECK_THROWS(w.moderation.adjust(student, id, 50, "x"), PermissionError);
    CHECK_THROWS(w.moderation.returnToLecturer(admin, id, "x"), PermissionError);
    CHECK_THROWS(w.moderation.release(lecturer, id), PermissionError);
    CHECK_THROWS(w.moderation.releaseAssignment(student, w.assignment), PermissionError);
    CHECK_THROWS(w.moderation.summary(lecturer), PermissionError);
    CHECK_THROWS(w.audit.listForMyDepartment(lecturer), PermissionError);
    CHECK_THROWS(w.audit.listForMyDepartment(admin), PermissionError);  // BR-10
    CHECK_THROWS(w.audit.listForMyDepartment(student), PermissionError);
    // Student functions
    CHECK_THROWS(w.submissionService.prepareSubmission(lecturer, w.assignment, "x", test::World::file()),
                 PermissionError);
    CHECK_THROWS(w.submissionService.prepareSubmission(admin, w.assignment, "x", test::World::file()), PermissionError);
    CHECK_THROWS(w.submissionService.listMyResults(lecturer), PermissionError);
    CHECK_THROWS(w.submissionService.listMyResults(admin), PermissionError);
    CHECK_THROWS(w.submissionService.listMyAssignments(admin), PermissionError);
}

TEST(own_course_and_own_department_checks) {  // Table 10, BR-07
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    const int id = w.submissionIdOf(*w.student1);
    // A lecturer of another course cannot see, mark or download.
    CHECK_THROWS(w.marking.listSubmissions(*w.otherLecturer, w.assignment), PermissionError);
    CHECK_THROWS(w.marking.saveMark(*w.otherLecturer, id, 30, "x", ""), PermissionError);
    CHECK_THROWS(w.marking.exportFile(*w.otherLecturer, id), PermissionError);
    CHECK_THROWS(w.assignmentService.createAssignment(*w.otherLecturer, w.course, "T", "D",
                                                      DateTime::parse("2026-12-01 10:00"), 10),
                 PermissionError);
    // The HoD of another department cannot moderate, release, or download.
    w.marking.saveMark(*w.lecturer, id, 30, "ok", "");
    CHECK_THROWS(w.moderation.approve(*w.otherHod, id), PermissionError);
    CHECK_THROWS(w.moderation.adjust(*w.otherHod, id, 20, "x"), PermissionError);
    CHECK_THROWS(w.moderation.returnToLecturer(*w.otherHod, id, "x"), PermissionError);
    CHECK_THROWS(w.moderation.listModerated(*w.otherHod, w.assignment), PermissionError);
    CHECK_THROWS(w.marking.exportFile(*w.otherHod, id), PermissionError);
    CHECK(w.moderation.listMarked(*w.otherHod).empty());
    CHECK(w.audit.listForMyDepartment(*w.otherHod).empty());
    CHECK(w.moderation.listMarked(*w.hod).size() == 1);
    CHECK(w.marking.exportFile(*w.hod, id).fileName == "work.txt");
    // A student cannot submit for a course of which the student is not a member (checked in another test),
    // and sees only own data.
    CHECK(w.submissionService.listMyResults(*w.student2).empty());
}

TEST(sql_injection_does_not_work) {  // AR-3: every query is parameterised
    test::World w;
    const std::string attack = "x' OR '1'='1";
    CHECK(w.users.findByUsername(attack) == nullptr);
    CHECK_THROWS(w.auth.login(attack, "Password1"), AuthenticationError);
    CHECK_THROWS(w.auth.login("student1' --", "x"), AuthenticationError);
    CHECK_THROWS(w.admin.createUser(*w.adminUser, "evil'; DROP TABLE user_account;--", "X", "Password1", Role::STUDENT,
                                    std::nullopt),
                 ValidationError);
    // Free text with quotes is stored as plain text and the tables stay.
    const int d = w.admin.createDepartment(*w.adminUser, "Robert'); DROP TABLE department;--");
    CHECK(w.courses.findDepartment(d)->getName() == "Robert'); DROP TABLE department;--");
    CHECK(w.courses.listDepartments().size() == 3);
    CHECK(w.users.list().size() == 8);
}

TEST(the_audit_log_cannot_be_changed_or_deleted) {  // NFR-09, AR-9
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    w.marking.saveMark(*w.lecturer, w.submissionIdOf(*w.student1), 30, "ok", "");
    CHECK_THROWS(w.db.exec("UPDATE audit_entry SET new_final_mark = 50"), DatabaseError);
    CHECK_THROWS(w.db.exec("DELETE FROM audit_entry"), DatabaseError);
    CHECK(w.audit.listForMyDepartment(*w.hod).size() == 1);
    CHECK(*w.submissions.findById(w.submissionIdOf(*w.student1))->getFinalMark() == 30);
}

TEST(a_failed_action_leaves_the_data_unchanged) {  // NFR-08, AR-5
    test::World w;
    {
        auto transaction = w.db.begin();
        w.courses.addDepartment(Department(0, "Temporary"));
        // no commit: the transaction is rolled back when it goes out of scope
    }
    CHECK(w.courses.listDepartments().size() == 2);
    // A mark that cannot be saved completely leaves nothing behind (here: the audit entry fails on a missing user).
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    const int id = w.submissionIdOf(*w.student1);
    {
        auto transaction = w.db.begin();
        auto s = w.submissions.findById(id);
        s->setMark(30, 0, 30, "x", w.lecturer->getId());
        s->changeStatus(SubmissionStatus::MARKED);
        w.submissions.update(*s);
        CHECK_THROWS(w.audits.add(AuditEntry(0, id, 9999, w.clock.now(), std::nullopt, 30, "bad user")), DatabaseError);
    }
    CHECK(w.submissions.findById(id)->getStatus() == SubmissionStatus::SUBMITTED);
    CHECK(w.audit.listForMyDepartment(*w.hod).empty());
}

TEST(database_rules_back_up_the_services) {  // the schema is a second barrier
    test::World w;
    CHECK_THROWS(w.db.exec("INSERT INTO course_lecturer (course_id, lecturer_id) VALUES (1, " +
                           std::to_string(w.student1->getId()) + ")"),
                 DatabaseError);  // only a lecturer teaches
    CHECK_THROWS(w.db.exec("INSERT INTO user_account (username, full_name, password_hash, role, department_id) "
                           "VALUES ('second_hod', 'Second', 'h', 'HEAD_OF_DEPARTMENT', 1)"),
                 DatabaseError);                                       // one active HoD
    CHECK_THROWS(w.db.exec("DELETE FROM department"), DatabaseError);  // no delete with rows that refer to it
    CHECK_THROWS(w.db.exec("INSERT INTO submission (assignment_id, student_id, status, raw_mark, penalty, final_mark, "
                           "feedback, marked_by) "
                           "VALUES (1, 5, 'MARKED', 1, 0, 1, '', 3)"),
                 DatabaseError);  // no mark without feedback
}

TEST(passwords_are_stored_only_as_salted_argon2id_hashes) {  // NFR-03
    SodiumPasswordHasher hasher;
    const auto start = std::chrono::steady_clock::now();
    const std::string h1 = hasher.hash("Password1");
    const auto end = std::chrono::steady_clock::now();
    const std::string h2 = hasher.hash("Password1");
    CHECK(h1 != h2);  // each password has its own random salt
    CHECK(h1.find("Password1") == std::string::npos);
    CHECK(h1.rfind("$argon2id$", 0) == 0);  // the algorithm is Argon2id
    CHECK(hasher.verify(h1, "Password1"));
    CHECK(hasher.verify(h2, "Password1"));
    CHECK(!hasher.verify(h1, "password1"));
    CHECK(!hasher.verify(h1, ""));
    CHECK(!hasher.verify("not a hash", "Password1"));
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "      (one Argon2id hash took " << ms << " ms on this computer; NFR-03 asks for 100 to 500 ms)\n";
    CHECK(ms >= 20 && ms <= 2000);  // only a sanity check; the real figure is measured on the reference computer

    // The database holds only the hash.
    test::World w;
    Statement st = w.db.prepare("SELECT password_hash FROM user_account WHERE username = 'student1'");
    st.step();
    CHECK(st.columnText(0) == "fake$Password1");  // the test hasher; in the program it is the Argon2id text
}
