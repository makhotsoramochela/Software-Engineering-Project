// Tests of the workflows: submit, late, resubmit, extension, mark, moderate, release, audit.
#include "TestSupport.h"
#include "World.h"

using namespace lms;

TEST(on_time_submission_has_status_submitted_and_a_receipt) {
    test::World w;
    w.clock.set("2026-11-10 11:59");
    const Receipt receipt = w.submit(*w.student1);
    CHECK(receipt.status == SubmissionStatus::SUBMITTED);
    CHECK(receipt.versionNo == 1);
    CHECK(receipt.daysLate == 0);
    CHECK(receipt.submittedAt.toDisplay() == "2026-11-10 11:59");
}

TEST(submission_exactly_at_the_deadline_is_on_time) {
    test::World w;
    w.clock.set("2026-11-10 12:00");
    CHECK(w.submit(*w.student1).status == SubmissionStatus::SUBMITTED);
}

TEST(late_work_is_accepted_with_a_warning_and_cannot_be_replaced) {  // FR-18, BR-03
    test::World w;
    w.clock.set("2026-11-11 12:01");  // 24 hours and 1 minute late: band 2
    const auto preview =
        w.submissionService.prepareSubmission(*w.student1, w.assignment, "late.txt", test::World::file());
    CHECK(preview.isLate);
    CHECK(preview.daysLate == 2);
    CHECK(preview.maxPenaltyMarks == 10);  // 2 x 10% of 50
    CHECK(!preview.getsZero);
    const Receipt receipt = w.submissionService.confirmSubmission(*w.student1, preview);
    CHECK(receipt.status == SubmissionStatus::LATE);
    CHECK(receipt.daysLate == 2);
    CHECK_THROWS(w.submissionService.prepareSubmission(*w.student1, w.assignment, "again.txt", test::World::file()),
                 RuleError);
}

TEST(work_more_than_72_hours_late_is_accepted_but_gets_zero) {
    test::World w;
    w.clock.set("2026-11-13 12:01");
    const auto preview = w.submissionService.prepareSubmission(*w.student1, w.assignment, "x.txt", test::World::file());
    CHECK(preview.getsZero);
    CHECK(preview.daysLate == 4);
    w.submissionService.confirmSubmission(*w.student1, preview);
    const int id = w.submissionIdOf(*w.student1);
    w.marking.saveMark(*w.lecturer, id, 40, "Good work, but very late.", "");
    const auto row = w.marking.listSubmissions(*w.lecturer, w.assignment).front();
    CHECK(*row.finalMark == 0);
}

TEST(resubmission_before_the_deadline_keeps_the_older_versions) {  // BR-03, FR-17
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1, "v1.txt");
    w.clock.set("2026-11-10 09:00");
    const Receipt second = w.submit(*w.student1, "v2.txt");
    CHECK(second.versionNo == 2);
    CHECK(second.status == SubmissionStatus::SUBMITTED);
    const int id = w.submissionIdOf(*w.student1);
    CHECK(w.submissions.latestVersion(id)->getFileName() == "v2.txt");
    const auto rows = w.marking.listSubmissions(*w.lecturer, w.assignment);
    CHECK(rows.size() == 1 && rows.front().versionCount == 2);
}

TEST(resubmission_after_the_deadline_or_after_marking_is_refused) {
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    w.clock.set("2026-11-10 12:01");
    CHECK_THROWS(w.submit(*w.student1), RuleError);  // the deadline has passed
    test::World w2;
    w2.clock.set("2026-11-09 10:00");
    w2.submit(*w2.student1);
    w2.marking.saveMark(*w2.lecturer, w2.submissionIdOf(*w2.student1), 30, "ok", "");
    w2.clock.set("2026-11-09 11:00");
    CHECK_THROWS(w2.submit(*w2.student1), RuleError);  // already marked
}

TEST(file_checks_empty_too_big_and_not_enrolled) {  // FR-16
    test::World w;
    w.clock.set("2026-11-09 10:00");
    CHECK_THROWS(w.submissionService.prepareSubmission(*w.student1, w.assignment, "e.txt", {}), ValidationError);
    CHECK_THROWS(
        w.submissionService.prepareSubmission(*w.student1, w.assignment, "big.bin", test::World::file(5242881)),
        ValidationError);
    w.submissionService.prepareSubmission(*w.student1, w.assignment, "max.bin", test::World::file(5242880));
    CHECK_THROWS(w.submissionService.prepareSubmission(*w.student1, w.assignment, "", test::World::file()),
                 ValidationError);
    CHECK_THROWS(w.submissionService.prepareSubmission(*w.student1, 9999, "x", test::World::file()), NotFoundError);
    // A student who is not enrolled cannot submit.
    w.admin.removeStudent(*w.adminUser, w.course, w.student3->getId());
    CHECK_THROWS(w.submit(*w.student3), PermissionError);
}

TEST(extension_makes_late_work_on_time_again) {  // BR-04
    test::World w;
    w.clock.set("2026-11-11 08:00");  // late by 20 hours: n = 1
    CHECK(w.submit(*w.student1).status == SubmissionStatus::LATE);
    w.clock.set("2026-11-11 09:00");
    w.assignmentService.grantExtension(*w.lecturer, w.assignment, w.student1->getId(),
                                       DateTime::parse("2026-11-12 12:00"), "Illness");
    const auto submission = w.submissions.find(w.assignment, w.student1->getId());
    CHECK(submission->getStatus() == SubmissionStatus::SUBMITTED);
    CHECK(submission->getDaysLate() == 0);
    CHECK(w.assignmentService.deadlineFor(w.assignment, w.student1->getId()) == DateTime::parse("2026-11-12 12:00"));
    // The other students keep the old deadline. The student can now resubmit by BR-03.
    CHECK(w.assignmentService.deadlineFor(w.assignment, w.student2->getId()) == DateTime::parse("2026-11-10 12:00"));
    CHECK(w.submit(*w.student1, "better.txt").versionNo == 2);
}

TEST(extension_recalculates_the_days_late_when_still_late) {
    test::World w;
    w.clock.set("2026-11-12 13:00");  // 2 days and 1 hour late: n = 3
    w.submit(*w.student1);
    w.assignmentService.grantExtension(*w.lecturer, w.assignment, w.student1->getId(),
                                       DateTime::parse("2026-11-11 12:00"), "Power cut");  // now n = 2
    const auto submission = w.submissions.find(w.assignment, w.student1->getId());
    CHECK(submission->getStatus() == SubmissionStatus::LATE);
    CHECK(submission->getDaysLate() == 2);
}

TEST(extension_rules) {  // BR-04, FR-10
    test::World w;
    const int s1 = w.student1->getId();
    CHECK_THROWS(
        w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-09 12:00"), "x"),
        RuleError);
    CHECK_THROWS(
        w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-10 12:00"), "x"),
        RuleError);
    CHECK_THROWS(
        w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-12 12:00"), "  "),
        ValidationError);
    CHECK_THROWS(w.assignmentService.grantExtension(*w.otherLecturer, w.assignment, s1,
                                                    DateTime::parse("2026-11-12 12:00"), "x"),
                 PermissionError);
    w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-12 12:00"), "First");
    CHECK_THROWS(w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-11 12:00"),
                                                    "Earlier"),
                 RuleError);
    w.assignmentService.grantExtension(*w.lecturer, w.assignment, s1, DateTime::parse("2026-11-14 12:00"), "Second");
    CHECK(w.assignmentService.deadlineFor(w.assignment, s1) == DateTime::parse("2026-11-14 12:00"));
    // No extension after the work was marked.
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student2);
    w.marking.saveMark(*w.lecturer, w.submissionIdOf(*w.student2), 30, "ok", "");
    CHECK_THROWS(w.assignmentService.grantExtension(*w.lecturer, w.assignment, w.student2->getId(),
                                                    DateTime::parse("2026-11-12 12:00"), "x"),
                 RuleError);
}

TEST(marking_applies_the_penalty_and_writes_one_audit_entry) {  // FR-12, FR-13, FR-14, BR-05
    test::World w;
    w.clock.set("2026-11-11 12:01");  // n = 2: penalty 10 of 50
    w.submit(*w.student1);
    const int id = w.submissionIdOf(*w.student1);
    const MarkPreview preview = w.marking.previewMark(*w.lecturer, id, 40);
    CHECK(preview.penalty == 10 && preview.finalMark == 30 && preview.daysLate == 2);
    CHECK_THROWS(w.marking.saveMark(*w.lecturer, id, 40, "   ", ""), ValidationError);  // no mark without feedback
    CHECK_THROWS(w.marking.saveMark(*w.lecturer, id, 51, "ok", ""), ValidationError);
    CHECK_THROWS(w.marking.saveMark(*w.lecturer, id, 40.55, "ok", ""), ValidationError);
    CHECK(w.submissions.findById(id)->getStatus() == SubmissionStatus::LATE);  // the failed saves changed nothing
    w.marking.saveMark(*w.lecturer, id, 40, "Well done.", "");
    const auto s = w.submissions.findById(id);
    CHECK(s->getStatus() == SubmissionStatus::MARKED);
    CHECK(*s->getRawMark() == 40 && *s->getPenalty() == 10 && *s->getFinalMark() == 30);
    CHECK(*s->getFeedback() == "Well done.");
    w.moderation.approve(*w.hod, id);
    const auto log = w.audit.listForMyDepartment(*w.hod);
    CHECK(log.size() == 1);
    CHECK(!log[0].getOldFinalMark().has_value());  // the first mark has no old final mark
    CHECK(log[0].getNewFinalMark() == 30);
    CHECK(log[0].getReason() == "initial marking");
    CHECK(log[0].getChangedBy() == w.lecturer->getId());
}

TEST(lecturer_can_change_a_mark_only_with_a_reason_and_only_while_marked_or_returned) {  // BR-06
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    const int id = w.submissionIdOf(*w.student1);
    w.marking.saveMark(*w.lecturer, id, 30, "First look.", "");
    CHECK_THROWS(w.marking.saveMark(*w.lecturer, id, 35, "Better.", ""), ValidationError);  // reason needed
    w.marking.saveMark(*w.lecturer, id, 35, "Better.", "Re-read the code");
    CHECK(*w.submissions.findById(id)->getFinalMark() == 35);
    w.moderation.approve(*w.hod, id);
    CHECK_THROWS(w.marking.saveMark(*w.lecturer, id, 40, "x", "why"), RuleError);  // Moderated: no longer allowed
    const auto log = w.audit.listForMyDepartment(*w.hod);
    CHECK(log.size() == 2);  // one entry for each change of the final mark
    CHECK(*log[0].getOldFinalMark() == 30 && log[0].getNewFinalMark() == 35 &&
          log[0].getReason() == "Re-read the code");
}

TEST(moderation_approve_adjust_return_and_release) {  // FR-20 to FR-23
    test::World w;
    w.clock.set("2026-11-09 10:00");
    for (auto* s : {w.student1.get(), w.student2.get(), w.student3.get()}) w.submit(*s);
    const int id1 = w.submissionIdOf(*w.student1), id2 = w.submissionIdOf(*w.student2),
              id3 = w.submissionIdOf(*w.student3);
    w.marking.saveMark(*w.lecturer, id1, 30, "ok", "");
    w.marking.saveMark(*w.lecturer, id2, 40, "very good", "");
    w.marking.saveMark(*w.lecturer, id3, 20, "weak", "");
    CHECK(w.moderation.listMarked(*w.hod).size() == 3);

    w.moderation.approve(*w.hod, id1);
    CHECK_THROWS(w.moderation.approve(*w.hod, id1), RuleError);  // not Marked any more
    w.moderation.adjust(*w.hod, id2, 38.5, "Rubric says 38.5");
    CHECK_THROWS(w.moderation.adjust(*w.hod, id3, 20, "same"), ValidationError);  // same mark: use Approve
    CHECK_THROWS(w.moderation.adjust(*w.hod, id3, 25, "  "), ValidationError);
    CHECK_THROWS(w.moderation.adjust(*w.hod, id3, 99, "too high"), ValidationError);
    w.moderation.returnToLecturer(*w.hod, id3, "Please check the penalty");
    CHECK(w.submissions.findById(id3)->getStatus() == SubmissionStatus::RETURNED);
    CHECK(*w.submissions.findById(id2)->getFinalMark() == 38.5);
    CHECK(*w.submissions.findById(id2)->getRawMark() == 40);  // the raw mark stays

    // The adjusted mark is in the audit log: old 40, new 38.5, with the reason.
    bool found = false;
    for (const auto& e : w.audit.listForMyDepartment(*w.hod)) {
        if (e.getSubmissionId() == id2 && e.getReason() == "Rubric says 38.5") {
            found = true;
            CHECK(*e.getOldFinalMark() == 40 && e.getNewFinalMark() == 38.5 && e.getChangedBy() == w.hod->getId());
        }
    }
    CHECK(found);

    // The lecturer sees a message at the next log in, then corrects the mark, then it is Marked again.
    const auto messages = w.messageService.takeWaiting(*w.lecturer);
    CHECK(messages.size() == 1);
    CHECK(w.messageService.takeWaiting(*w.lecturer).empty());  // a message is shown only once
    w.marking.saveMark(*w.lecturer, id3, 22, "Penalty checked.", "HoD comment");
    CHECK(w.submissions.findById(id3)->getStatus() == SubmissionStatus::MARKED);

    // Only Moderated work can be released (BR-07). Marked work cannot.
    CHECK_THROWS(w.moderation.release(*w.hod, id3), RuleError);
    w.moderation.release(*w.hod, id1);
    CHECK(w.moderation.releaseAssignment(*w.hod, w.assignment) == 1);  // id2 was the only one left Moderated
    CHECK(w.submissions.findById(id2)->getStatus() == SubmissionStatus::RELEASED);
    CHECK(w.submissions.findById(id3)->getStatus() == SubmissionStatus::MARKED);
    CHECK(w.moderation.releaseAssignment(*w.hod, w.assignment) == 0);
    CHECK_THROWS(w.moderation.release(*w.hod, id1), RuleError);  // Released is the last status

    // The students whose marks were released have a message.
    CHECK(w.messageService.takeWaiting(*w.student1).size() == 1);
    CHECK(w.messageService.takeWaiting(*w.student3).empty());

    const auto summary = w.moderation.summary(*w.hod);
    CHECK(summary.size() == 1 && summary[0].marked == 1 && summary[0].moderated == 0 && summary[0].released == 2);
}

TEST(students_see_no_mark_before_release_and_the_mark_after_release) {  // BR-02, FR-19
    test::World w;
    w.clock.set("2026-11-09 10:00");
    w.submit(*w.student1);
    const int id = w.submissionIdOf(*w.student1);
    w.marking.saveMark(*w.lecturer, id, 30, "Feedback text", "");
    CHECK(w.submissionService.listMyResults(*w.student1).empty());
    CHECK(w.submissionService.listMyAssignments(*w.student1).front().statusText == "Submitted");  // not "Marked"
    w.moderation.approve(*w.hod, id);
    CHECK(w.submissionService.listMyResults(*w.student1).empty());
    CHECK(w.submissionService.listMyAssignments(*w.student1).front().statusText == "Submitted");
    w.moderation.release(*w.hod, id);
    const auto results = w.submissionService.listMyResults(*w.student1);
    CHECK(results.size() == 1 && results[0].finalMark == 30 && results[0].feedback == "Feedback text");
    CHECK(w.submissionService.listMyAssignments(*w.student1).front().statusText == "Released");
    CHECK(w.submissionService.listMyResults(*w.student2).empty());  // another student sees nothing of it
}

TEST(student_assignment_list_shows_the_deadline_of_the_student_and_not_submitted) {  // FR-15
    test::World w;
    w.assignmentService.grantExtension(*w.lecturer, w.assignment, w.student1->getId(),
                                       DateTime::parse("2026-11-12 12:00"), "Illness");
    const auto rows1 = w.submissionService.listMyAssignments(*w.student1);
    const auto rows2 = w.submissionService.listMyAssignments(*w.student2);
    CHECK(rows1.size() == 1 && rows1[0].deadline == DateTime::parse("2026-11-12 12:00"));
    CHECK(rows2.size() == 1 && rows2[0].deadline == DateTime::parse("2026-11-10 12:00"));
    CHECK(rows1[0].statusText == "Not submitted");
    CHECK(rows1[0].courseCode == "CS4433");
}

TEST(administrator_rules_for_users_departments_and_courses) {  // FR-04 to FR-08
    test::World w;
    const User& a = *w.adminUser;
    CHECK_THROWS(w.admin.createUser(a, "student1", "Duplicate", "Password1", Role::STUDENT, std::nullopt), RuleError);
    CHECK_THROWS(w.admin.createUser(a, "STUDENT1", "Duplicate", "Password1", Role::STUDENT, std::nullopt),
                 RuleError);  // not case sensitive
    CHECK_THROWS(w.admin.createUser(a, "newhod", "New Head", "Password1", Role::HEAD_OF_DEPARTMENT, w.csDepartment),
                 RuleError);  // one active HoD
    CHECK_THROWS(w.admin.createUser(a, "newhod", "New Head", "Password1", Role::HEAD_OF_DEPARTMENT, std::nullopt),
                 ValidationError);
    CHECK_THROWS(w.admin.createUser(a, "stud", "Student", "Password1", Role::STUDENT, w.csDepartment), ValidationError);
    CHECK_THROWS(w.admin.createUser(a, "weakpw", "Weak", "weak", Role::STUDENT, std::nullopt), ValidationError);
    w.admin.deactivateUser(a, w.hod->getId());  // frees the department
    w.admin.createUser(a, "newhod", "New Head", "Password1", Role::HEAD_OF_DEPARTMENT, w.csDepartment);
    CHECK_THROWS(w.admin.deactivateUser(a, a.getId()), RuleError);  // not the own account
    CHECK_THROWS(w.admin.createDepartment(a, "computer science"), RuleError);
    CHECK_THROWS(w.admin.createCourse(a, "CS4433", "Again", w.csDepartment), RuleError);
    CHECK_THROWS(w.admin.assignLecturer(a, w.course, w.student1->getId()), RuleError);  // only a lecturer
    CHECK_THROWS(w.admin.enrolStudent(a, w.course, w.lecturer->getId()), RuleError);    // only a student
    CHECK_THROWS(w.admin.enrolStudent(a, w.course, w.student1->getId()), RuleError);    // already enrolled
    w.admin.removeStudent(a, w.course, w.student1->getId());
    CHECK(!w.courses.isEnrolled(w.course, w.student1->getId()));
    CHECK_THROWS(w.admin.removeStudent(a, w.course, w.student1->getId()), NotFoundError);
    CHECK_THROWS(w.admin.deactivateDepartment(a, w.mathsDepartment),
                 RuleError);                                        // it still has an active course and a Head
    w.admin.assignLecturer(a, w.course, w.otherLecturer->getId());  // more than one lecturer for a course
    CHECK(w.courses.lecturerTeaches(w.course, w.otherLecturer->getId()));
}
