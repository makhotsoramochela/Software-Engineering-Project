#include "ModerationService.h"

#include <cmath>

#include "Exceptions.h"
#include "Validation.h"

namespace lms {

namespace {

// How many submissions have this status. A status that is not in the map has none.
int countOf(const std::map<SubmissionStatus, int>& counts, SubmissionStatus status) {
    const auto found = counts.find(status);
    if (found == counts.end()) return 0;
    return found->second;
}

}  // namespace

ModerationService::ModerationService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                                     ICourseRepository& courses, AuditService& audit, MessageService& messages,
                                     Clock& clock, UnitOfWork& uow)
    : submissions_(submissions),
      assignments_(assignments),
      courses_(courses),
      audit_(audit),
      messages_(messages),
      clock_(clock),
      uow_(uow) {}

// BR-07: a Head of Department works only with the courses of the own department.
ModerationService::Loaded ModerationService::requireSubmissionOfDepartment(const User& actor, int submissionId) {
    requirePermission(actor, Function::MODERATE_MARKS);
    auto submission = submissions_.findById(submissionId);
    if (!submission) throw NotFoundError("Submission " + std::to_string(submissionId) + " does not exist.");
    auto assignment = assignments_.findById(submission->getAssignmentId());
    if (!assignment) throw NotFoundError("The assignment of this submission does not exist.");
    auto course = courses_.findCourse(assignment->getCourseId());
    if (!course) throw NotFoundError("The course of this submission does not exist.");
    if (course->getDepartmentId() != departmentOf(actor)) {
        throw PermissionError("This submission is not in your department.");
    }
    return Loaded{*submission, *assignment, *course};
}

std::vector<SubmissionRow> ModerationService::listMarked(const User& actor) {
    requirePermission(actor, Function::MODERATE_MARKS);
    return submissions_.listByStatus(SubmissionStatus::MARKED, departmentOf(actor));
}

void ModerationService::approve(const User& actor, int submissionId) {
    Loaded loaded = requireSubmissionOfDepartment(actor, submissionId);
    if (loaded.submission.getStatus() != SubmissionStatus::MARKED) {
        throw RuleError("Only a Marked submission can be approved.");
    }
    auto transaction = uow_.begin();
    loaded.submission.changeStatus(SubmissionStatus::MODERATED);
    submissions_.update(loaded.submission);
    submissions_.addStatusChange(StatusChange(0, submissionId, SubmissionStatus::MARKED, SubmissionStatus::MODERATED,
                                              actor.getId(), clock_.now(), std::string("Approved")));
    transaction->commit();
}

void ModerationService::adjust(const User& actor, int submissionId, double newFinal, const std::string& reason) {
    Loaded loaded = requireSubmissionOfDepartment(actor, submissionId);
    if (loaded.submission.getStatus() != SubmissionStatus::MARKED) {
        throw RuleError("Only a Marked submission can be adjusted.");  // BR-06
    }
    validate::mark(newFinal, loaded.assignment.getMaxMark());
    const std::string cleanReason = validate::text(reason, "The reason", 500);
    const std::optional<double> oldFinal = loaded.submission.getFinalMark();
    if (oldFinal && std::fabs(*oldFinal - newFinal) < 1e-9) {
        throw ValidationError("The new final mark is the same as the old one. Use Approve instead.");
    }
    auto transaction = uow_.begin();
    loaded.submission.setFinalMark(newFinal);
    loaded.submission.changeStatus(SubmissionStatus::MODERATED);
    submissions_.update(loaded.submission);
    submissions_.addStatusChange(StatusChange(0, submissionId, SubmissionStatus::MARKED, SubmissionStatus::MODERATED,
                                              actor.getId(), clock_.now(), "Final mark adjusted: " + cleanReason));
    audit_.record(actor, submissionId, oldFinal, newFinal, cleanReason);  // FR-14: same transaction
    transaction->commit();
}

void ModerationService::returnToLecturer(const User& actor, int submissionId, const std::string& comment) {
    Loaded loaded = requireSubmissionOfDepartment(actor, submissionId);
    if (loaded.submission.getStatus() != SubmissionStatus::MARKED) {
        throw RuleError("Only a Marked submission can be sent back.");
    }
    const std::string cleanComment = validate::text(comment, "The comment", 500);
    auto transaction = uow_.begin();
    loaded.submission.changeStatus(SubmissionStatus::RETURNED);
    submissions_.update(loaded.submission);
    submissions_.addStatusChange(StatusChange(0, submissionId, SubmissionStatus::MARKED, SubmissionStatus::RETURNED,
                                              actor.getId(), clock_.now(), cleanComment));
    const std::optional<int> marker = loaded.submission.getMarkedBy();
    if (marker) {  // FR-23: the lecturer sees a message at the next log in
        messages_.notify(*marker, "Submission " + std::to_string(submissionId) + " of " + loaded.assignment.getTitle() +
                                      " (" + loaded.course.getCode() +
                                      ") was sent back by the Head of Department: " + cleanComment);
    }
    transaction->commit();
}

std::vector<SubmissionRow> ModerationService::listModerated(const User& actor, int assignmentId) {
    requirePermission(actor, Function::RELEASE_MARKS);
    auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment " + std::to_string(assignmentId) + " does not exist.");
    auto course = courses_.findCourse(assignment->getCourseId());
    if (!course || course->getDepartmentId() != departmentOf(actor)) {
        throw PermissionError("This assignment is not in your department.");
    }
    std::vector<SubmissionRow> rows;
    for (SubmissionRow& row : submissions_.listByAssignment(assignmentId)) {
        if (row.status == SubmissionStatus::MODERATED) rows.push_back(std::move(row));
    }
    return rows;
}

// Must be called inside a transaction.
void ModerationService::releaseInside(const User& actor, Loaded& loaded) {
    if (loaded.submission.getStatus() != SubmissionStatus::MODERATED) {
        throw RuleError("Only a Moderated submission can be released.");  // BR-07
    }
    const int id = loaded.submission.getId();
    loaded.submission.changeStatus(SubmissionStatus::RELEASED);
    submissions_.update(loaded.submission);
    submissions_.addStatusChange(
        StatusChange(0, id, SubmissionStatus::MODERATED, SubmissionStatus::RELEASED, actor.getId(), clock_.now()));
    const std::string what = loaded.assignment.getTitle() + " (" + loaded.course.getCode() + ")";
    messages_.notify(loaded.submission.getStudentId(), "Your mark for " + what + " has been released.");
    const std::optional<int> marker = loaded.submission.getMarkedBy();
    if (marker) {
        messages_.notify(*marker,
                         "The mark of submission " + std::to_string(id) + " of " + what + " has been released.");
    }
}

void ModerationService::release(const User& actor, int submissionId) {
    Loaded loaded = requireSubmissionOfDepartment(actor, submissionId);
    requirePermission(actor, Function::RELEASE_MARKS);
    auto transaction = uow_.begin();
    releaseInside(actor, loaded);
    transaction->commit();
}

int ModerationService::releaseAssignment(const User& actor, int assignmentId) {
    const std::vector<SubmissionRow> rows = listModerated(actor, assignmentId);  // permission and department
    auto transaction = uow_.begin();                                             // all or none
    for (const SubmissionRow& row : rows) {
        Loaded loaded = requireSubmissionOfDepartment(actor, row.submissionId);
        releaseInside(actor, loaded);
    }
    transaction->commit();
    return static_cast<int>(rows.size());
}

std::vector<AssignmentSummary> ModerationService::summary(const User& actor) {
    requirePermission(actor, Function::MODERATE_MARKS);
    std::vector<AssignmentSummary> rows;
    for (const Course& course : courses_.listCoursesOfDepartment(departmentOf(actor))) {
        for (const Assignment& a : assignments_.listByCourse(course.getId())) {
            const std::map<SubmissionStatus, int> counts = submissions_.countByStatus(a.getId());
            AssignmentSummary row;
            row.assignmentId = a.getId();
            row.courseCode = course.getCode();
            row.title = a.getTitle();
            row.marked = countOf(counts, SubmissionStatus::MARKED);
            row.moderated = countOf(counts, SubmissionStatus::MODERATED);
            row.released = countOf(counts, SubmissionStatus::RELEASED);
            rows.push_back(row);
        }
    }
    return rows;
}

}  // namespace lms
