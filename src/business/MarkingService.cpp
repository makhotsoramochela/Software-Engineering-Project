#include "MarkingService.h"

#include <cmath>

#include "Exceptions.h"
#include "Validation.h"

namespace lms {

MarkingService::MarkingService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                               ICourseRepository& courses, PenaltyStrategy& penalty, AuditService& audit, Clock& clock,
                               UnitOfWork& uow)
    : submissions_(submissions),
      assignments_(assignments),
      courses_(courses),
      penalty_(penalty),
      audit_(audit),
      clock_(clock),
      uow_(uow) {}

MarkingService::Loaded MarkingService::requireSubmissionOfLecturer(const User& actor, int submissionId) {
    auto submission = submissions_.findById(submissionId);
    if (!submission) throw NotFoundError("Submission " + std::to_string(submissionId) + " does not exist.");
    auto assignment = assignments_.findById(submission->getAssignmentId());
    if (!assignment) throw NotFoundError("The assignment of this submission does not exist.");
    if (!courses_.lecturerTeaches(assignment->getCourseId(), actor.getId())) {
        throw PermissionError("You do not teach the course of this submission.");
    }
    return Loaded{*submission, *assignment};
}

std::vector<SubmissionRow> MarkingService::listSubmissions(const User& actor, int assignmentId) {
    requirePermission(actor, Function::MARK_SUBMISSION);
    auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment " + std::to_string(assignmentId) + " does not exist.");
    if (!courses_.lecturerTeaches(assignment->getCourseId(), actor.getId())) {
        throw PermissionError("You do not teach the course of this assignment.");
    }
    return submissions_.listByAssignment(assignmentId);
}

MarkPreview MarkingService::previewMark(const User& actor, int submissionId, double rawMark) {
    requirePermission(actor, Function::MARK_SUBMISSION);
    const Loaded loaded = requireSubmissionOfLecturer(actor, submissionId);
    const SubmissionStatus status = loaded.submission.getStatus();
    // BR-06: a lecturer can mark Submitted and Late work and can change a mark only while it is Marked or Returned.
    if (status != SubmissionStatus::SUBMITTED && status != SubmissionStatus::LATE &&
        status != SubmissionStatus::MARKED && status != SubmissionStatus::RETURNED) {
        throw RuleError("A submission with the status " + displayName(status) + " cannot be marked by the lecturer.");
    }
    validate::mark(rawMark, loaded.assignment.getMaxMark());
    const PenaltyResult result =
        penalty_.apply(rawMark, loaded.assignment.getMaxMark(), loaded.submission.getDaysLate());
    MarkPreview preview;
    preview.rawMark = rawMark;
    preview.penalty = result.penalty;
    preview.finalMark = result.finalMark;
    preview.daysLate = loaded.submission.getDaysLate();
    preview.maxMark = loaded.assignment.getMaxMark();
    return preview;
}

void MarkingService::saveMark(const User& actor, int submissionId, double rawMark, const std::string& feedback,
                              const std::string& reason) {
    const MarkPreview preview = previewMark(actor, submissionId, rawMark);  // permission, status, mark checks
    Loaded loaded = requireSubmissionOfLecturer(actor, submissionId);
    Submission& submission = loaded.submission;
    const std::string cleanFeedback = validate::text(feedback, "The feedback", 2000);  // FR-12
    const SubmissionStatus before = submission.getStatus();
    const bool firstMark = before == SubmissionStatus::SUBMITTED || before == SubmissionStatus::LATE;
    const std::string cleanReason = firstMark ? "initial marking" : validate::text(reason, "The reason", 500);
    const std::optional<double> oldFinal = submission.getFinalMark();

    auto transaction = uow_.begin();
    submission.setMark(preview.rawMark, preview.penalty, preview.finalMark, cleanFeedback, actor.getId());
    if (before != SubmissionStatus::MARKED) {
        submission.changeStatus(SubmissionStatus::MARKED);
        submissions_.addStatusChange(
            StatusChange(0, submissionId, before, SubmissionStatus::MARKED, actor.getId(), clock_.now(), cleanReason));
    }
    submissions_.update(submission);
    const bool finalChanged = !oldFinal || std::fabs(*oldFinal - preview.finalMark) > 1e-9;
    if (finalChanged) {  // NFR-09: every change of a final mark has exactly one audit entry
        audit_.record(actor, submissionId, oldFinal, preview.finalMark, cleanReason);
    }
    transaction->commit();
}

StoredFile MarkingService::exportFile(const User& actor, int submissionId) {
    auto submission = submissions_.findById(submissionId);
    if (!submission) throw NotFoundError("Submission " + std::to_string(submissionId) + " does not exist.");
    auto assignment = assignments_.findById(submission->getAssignmentId());
    if (!assignment) throw NotFoundError("The assignment of this submission does not exist.");
    if (actor.can(Function::MARK_SUBMISSION)) {
        if (!courses_.lecturerTeaches(assignment->getCourseId(), actor.getId())) {
            throw PermissionError("You do not teach the course of this submission.");
        }
    } else if (actor.can(Function::MODERATE_MARKS)) {
        auto course = courses_.findCourse(assignment->getCourseId());
        if (!course || course->getDepartmentId() != departmentOf(actor)) {
            throw PermissionError("This submission is not in your department.");
        }
    } else {
        throw PermissionError("Your role is not allowed to do this.");
    }
    auto version = submissions_.latestVersion(submissionId);
    if (!version) throw NotFoundError("This submission has no file.");
    return StoredFile{version->getFileName(), version->getContent()};
}

}  // namespace lms
