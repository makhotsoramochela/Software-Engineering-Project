// Timing check for NFR-01, NFR-02 and NFR-13. It is not part of ctest, because the figures depend on the computer.
// Run: build/tests/lms_bench      (it uses the real Argon2id hasher and a database with the NFR-13 size)
#include <chrono>
#include <iostream>

#include "SodiumPasswordHasher.h"
#include "World.h"

using Clock_ = std::chrono::steady_clock;
static double msSince(Clock_::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock_::now() - start).count();
}

int main() {
    using namespace lms;
    test::World w;
    SodiumPasswordHasher realHasher;
    AuthService realAuth(w.users, realHasher, w.clock, w.db);

    // Fill the database to the size of NFR-13 with plain SQL: 1,000 students, 100 courses, 20,000 submissions.
    const std::string hash = realHasher.hash("Password1");
    {
        auto transaction = w.db.begin();
        for (int i = 0; i < 1000; ++i) {
            Statement st = w.db.prepare(
                "INSERT INTO user_account (username, full_name, password_hash, role) "
                "VALUES (?, ?, ?, 'STUDENT')");
            st.bindText(1, "bench" + std::to_string(i));
            st.bindText(2, "Bench Student " + std::to_string(i));
            st.bindText(3, hash);
            st.step();
        }
        w.db.exec(
            "INSERT INTO department (name) SELECT 'D' || value FROM (WITH RECURSIVE n(value) AS (SELECT 1 UNION ALL "
            "SELECT value + 1 FROM n WHERE value < 5) SELECT value FROM n)");
        for (int c = 0; c < 100; ++c) {
            Statement st = w.db.prepare("INSERT INTO course (code, title, department_id) VALUES (?, ?, 1)");
            st.bindText(1, "BC" + std::to_string(c));
            st.bindText(2, "Bench course " + std::to_string(c));
            st.step();
        }
        // 25 assignments, each with 800 submissions of 1 KB: 20,000 submissions.
        for (int a = 0; a < 25; ++a) {
            Statement st = w.db.prepare(
                "INSERT INTO assignment (course_id, title, description, deadline, max_mark) "
                "VALUES (?, ?, 'x', '2026-12-01T12:00', 50)");
            st.bindInt(1, 3 + a);
            st.bindText(2, "Bench assignment " + std::to_string(a));
            st.step();
        }
        w.db.exec(
            "INSERT INTO submission (assignment_id, student_id, status) "
            "SELECT a.assignment_id, u.user_id, 'SUBMITTED' FROM assignment a JOIN user_account u "
            "ON u.username LIKE 'bench%' WHERE a.title LIKE 'Bench assignment%' AND "
            "CAST(substr(u.username, 6) AS INTEGER) < 800");
        w.db.exec(
            "INSERT INTO submission_version (submission_id, version_no, file_name, file_content, submitted_at) "
            "SELECT submission_id, 1, 'f.txt', zeroblob(1024), '2026-11-30T10:00' FROM submission "
            "WHERE assignment_id IN (SELECT assignment_id FROM assignment WHERE title LIKE 'Bench assignment%')");
        transaction->commit();
    }
    Statement count = w.db.prepare(
        "SELECT (SELECT COUNT(*) FROM user_account), (SELECT COUNT(*) FROM course), "
        "(SELECT COUNT(*) FROM submission)");
    count.step();
    std::cout << "Database: " << count.columnInt(0) << " users, " << count.columnInt(1) << " courses, "
              << count.columnInt(2) << " submissions\n";

    // NFR-01: logging in takes at most 2 seconds in at least 29 of 30 timed attempts.
    int over = 0;
    double total = 0, worst = 0;
    for (int i = 0; i < 30; ++i) {
        const auto start = Clock_::now();
        auto user = realAuth.login("bench" + std::to_string(i * 7), "Password1");
        const double ms = msSince(start);
        total += ms;
        worst = std::max(worst, ms);
        if (ms > 2000) ++over;
    }
    std::cout << "NFR-01 log in:        average " << total / 30 << " ms, worst " << worst
              << " ms, attempts over 2 s: " << over << " of 30  (limit: 1)\n";

    // NFR-02: save a 5 MB submission, save a mark, show a list of 800 submissions: each at most 2 seconds.
    auto student = w.users.findByUsername("bench900");
    w.admin.enrolStudent(*w.adminUser, w.course, student->getId());
    w.clock.set("2026-11-09 10:00");
    auto start = Clock_::now();
    const auto preview =
        w.submissionService.prepareSubmission(*student, w.assignment, "big.bin", test::World::file(5242880));
    w.submissionService.confirmSubmission(*student, preview);
    std::cout << "NFR-02 save 5 MB:     " << msSince(start) << " ms  (limit: 2000)\n";

    const int id = w.submissionIdOf(*student);
    start = Clock_::now();
    w.marking.saveMark(*w.lecturer, id, 40, "Bench feedback", "");
    std::cout << "NFR-02 save a mark:   " << msSince(start) << " ms  (limit: 2000)\n";

    // The lecturer must teach the course of the assignment with 800 submissions.
    Statement big = w.db.prepare("SELECT assignment_id, course_id FROM assignment WHERE title = 'Bench assignment 0'");
    big.step();
    w.admin.assignLecturer(*w.adminUser, static_cast<int>(big.columnInt(1)), w.lecturer->getId());
    start = Clock_::now();
    const auto rows = w.marking.listSubmissions(*w.lecturer, static_cast<int>(big.columnInt(0)));
    std::cout << "NFR-02 list " << rows.size() << " submissions: " << msSince(start) << " ms  (limit: 2000)\n";
    return 0;
}
