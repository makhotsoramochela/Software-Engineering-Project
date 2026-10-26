// Tests of the pure rules: dates, the late penalty (BR-01, Table 8), status changes (Table 9), validation.
#include "DateTime.h"
#include "Entities.h"
#include "PenaltyStrategy.h"
#include "TestSupport.h"
#include "Validation.h"

using namespace lms;

TEST(datetime_parses_and_prints) {
    const DateTime t = DateTime::parse("2026-11-10 12:00");
    CHECK(t.toString() == "2026-11-10T12:00");
    CHECK(t.toDisplay() == "2026-11-10 12:00");
    CHECK(DateTime::parse("2026-11-10T12:00") == t);
    CHECK(t.plusMinutes(1).toDisplay() == "2026-11-10 12:01");
    CHECK(t.plusMinutes(24 * 60).toDisplay() == "2026-11-11 12:00");
    CHECK(DateTime::parse("2028-02-29 23:59").plusMinutes(1).toDisplay() == "2028-03-01 00:00");  // leap year
}

TEST(datetime_rejects_wrong_input) {
    CHECK_THROWS(DateTime::parse(""), ValidationError);
    CHECK_THROWS(DateTime::parse("tomorrow"), ValidationError);
    CHECK_THROWS(DateTime::parse("2026-13-01 10:00"), ValidationError);
    CHECK_THROWS(DateTime::parse("2026-02-30 10:00"), ValidationError);
    CHECK_THROWS(DateTime::parse("2027-02-29 10:00"), ValidationError);
    CHECK_THROWS(DateTime::parse("2026-11-10 24:00"), ValidationError);
    CHECK_THROWS(DateTime::parse("2026-11-10 12:60"), ValidationError);
    CHECK_THROWS(DateTime::parse("2026-11-10 12:00 extra"), ValidationError);
}

// Table 8 of the SRS: M = 50, R = 40, and the boundary times that are used in the tests.
TEST(late_penalty_bands_and_boundaries) {
    TenPercentPerDayPenalty strategy;
    const DateTime due = DateTime::parse("2026-11-10 12:00");
    struct Row {
        long long minutesAfter;
        int days;
        double penalty;
        double finalMark;
    };
    const Row rows[] = {
        {-30, 0, 0, 40},
        {0, 0, 0, 40},
        {1, 1, 5, 35},
        {24 * 60, 1, 5, 35},
        {24 * 60 + 1, 2, 10, 30},
        {48 * 60, 2, 10, 30},
        {48 * 60 + 1, 3, 15, 25},
        {72 * 60, 3, 15, 25},
        {72 * 60 + 1, 4, 40, 0},
        {10 * 24 * 60, 10, 40, 0},
    };
    for (const Row& row : rows) {
        const int days = strategy.daysLate(due, due.plusMinutes(row.minutesAfter));
        CHECK(days == row.days);
        const PenaltyResult r = strategy.apply(40, 50, days);
        CHECK(r.penalty == row.penalty);
        CHECK(r.finalMark == row.finalMark);
    }
}

TEST(late_penalty_never_gives_a_negative_mark) {
    TenPercentPerDayPenalty strategy;
    const PenaltyResult r = strategy.apply(10, 50, 3);  // the penalty (15) is bigger than the raw mark (10)
    CHECK(r.penalty == 15);
    CHECK(r.finalMark == 0);
}

TEST(late_penalty_keeps_one_decimal_without_rounding_error) {
    TenPercentPerDayPenalty strategy;
    const PenaltyResult r = strategy.apply(37.3, 33, 2);  // penalty = 2 x 3.3 = 6.6
    CHECK(r.penalty == 6.6);
    CHECK(r.finalMark == 30.7);
}

// Table 9 of the SRS: the only allowed changes of status.
TEST(status_changes_follow_table_9) {
    using S = SubmissionStatus;
    const S all[] = {S::SUBMITTED, S::LATE, S::MARKED, S::RETURNED, S::MODERATED, S::RELEASED};
    int allowed = 0;
    for (const S from : all) {
        for (const S to : all) {
            if (Submission::isAllowedChange(from, to)) ++allowed;
        }
    }
    CHECK(allowed == 7);
    CHECK(Submission::isAllowedChange(S::SUBMITTED, S::MARKED));
    CHECK(Submission::isAllowedChange(S::LATE, S::MARKED));
    CHECK(Submission::isAllowedChange(S::LATE, S::SUBMITTED));
    CHECK(Submission::isAllowedChange(S::MARKED, S::MODERATED));
    CHECK(Submission::isAllowedChange(S::MARKED, S::RETURNED));
    CHECK(Submission::isAllowedChange(S::RETURNED, S::MARKED));
    CHECK(Submission::isAllowedChange(S::MODERATED, S::RELEASED));
    CHECK(!Submission::isAllowedChange(S::SUBMITTED, S::RELEASED));
    CHECK(!Submission::isAllowedChange(S::RELEASED, S::MARKED));

    Submission s(1, 1, 1, S::SUBMITTED);
    CHECK_THROWS(s.changeStatus(S::RELEASED), RuleError);
    s.changeStatus(S::MARKED);
    CHECK(s.getStatus() == S::MARKED);
}

TEST(students_see_only_submitted_or_late_until_release) {  // BR-02
    using S = SubmissionStatus;
    Submission onTime(1, 1, 1, S::MARKED, 0);
    Submission late(2, 1, 1, S::MODERATED, 2);
    Submission released(3, 1, 1, S::RELEASED, 2);
    CHECK(onTime.visibleStatus() == S::SUBMITTED);
    CHECK(late.visibleStatus() == S::LATE);
    CHECK(released.visibleStatus() == S::RELEASED);
    CHECK(!onTime.isVisibleToStudent());
    CHECK(released.isVisibleToStudent());
}

TEST(roles_own_only_the_functions_of_table_10) {
    Administrator admin(1, "a", "A", "h");
    Lecturer lecturer(2, "l", "L", "h");
    HeadOfDepartment head(3, "h", "H", "h", 1);
    Student student(4, "s", "S", "h");
    CHECK(admin.can(Function::MANAGE_USERS) && !admin.can(Function::VIEW_AUDIT_LOG));
    CHECK(!admin.can(Function::MARK_SUBMISSION) && !admin.can(Function::VIEW_RELEASED_MARKS));  // BR-10
    CHECK(lecturer.can(Function::MARK_SUBMISSION) && !lecturer.can(Function::RELEASE_MARKS));
    CHECK(head.can(Function::RELEASE_MARKS) && head.can(Function::VIEW_AUDIT_LOG) &&
          !head.can(Function::MARK_SUBMISSION));
    CHECK(student.can(Function::SUBMIT_WORK) && !student.can(Function::MANAGE_USERS));
    CHECK(admin.role() == Role::ADMINISTRATOR && lecturer.role() == Role::LECTURER);
    CHECK(head.role() == Role::HEAD_OF_DEPARTMENT && student.role() == Role::STUDENT);
}

TEST(validation_rules) {
    CHECK_THROWS(validate::password("short1"), ValidationError);        // BR-08: under 8 characters
    CHECK_THROWS(validate::password("longpassword"), ValidationError);  // no digit
    CHECK_THROWS(validate::password("12345678"), ValidationError);      // no letter
    validate::password("Password1");
    CHECK_THROWS(validate::mark(-1, 50), ValidationError);  // BR-09
    CHECK_THROWS(validate::mark(50.1, 50), ValidationError);
    CHECK_THROWS(validate::mark(40.55, 50), ValidationError);  // two decimal places
    validate::mark(40.5, 50);
    validate::mark(0, 50);
    validate::mark(50, 50);
    CHECK_THROWS(validate::maxMark(0), ValidationError);
    CHECK_THROWS(validate::maxMark(101), ValidationError);
    CHECK_THROWS(validate::text("   ", "Field", 10), ValidationError);
    CHECK_THROWS(validate::text("12345678901", "Field", 10), ValidationError);
    CHECK(validate::text("  hello ", "Field", 10) == "hello");
    CHECK_THROWS(validate::username("a b"), ValidationError);
    CHECK_THROWS(validate::username("x' OR '1'='1"), ValidationError);
}
