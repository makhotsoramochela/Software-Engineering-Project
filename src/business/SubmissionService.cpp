#include "SubmissionService.h"

#include "Exceptions.h"
#include "Validation.h"

namespace lms {

SubmissionService::SubmissionService(ISubmissionRepository& submissions, IAssignmentRepository& assignments,
                                     ICourseRepository& courses, AssignmentService& deadlines, PenaltyStrategy& penalty,
                                     Clock& clock, UnitOfWork& uow)
    : submissions_(submissions),
      assignments_(assignments),
      courses_(courses),
      deadlines_(deadlines),
      penalty_(penalty),
      clock_(clock),
      uow_(uow) {}

std::vector<StudentAssignmentRow> SubmissionService::listMyAssignments(const User& actor) {
    requirePermission(actor, Function::VIEW_COURSES);
    std::vector<StudentAssignmentRow> rows;
    for (const Course& course : courses_.listCoursesOfStudent(actor.getId())) {
        for (const Assignment& a : assignments_.listByCourse(course.getId())) {
            StudentAssignmentRow row;
            row.assignmentId = a.getId();
            row.courseCode = course.getCode();
            row.courseTitle = course.getTitle();
            row.title = a.getTitle();
            row.deadline = deadlines_.deadlineFor(a.getId(), actor.getId());
            row.maxMark = a.getMaxMark();
            auto submission = submissions_.find(a.getId(), actor.getId());
            row.statusText = submission ? displayName(submission->visibleStatus()) : "Not submitted";
            rows.push_back(row);
        }
    }
    return rows;
}

SubmissionPreview SubmissionService::prepareSubmission(const User& actor, int assignmentId, const std::string& fileName,
                                                       const std::vector<std::uint8_t>& content) {
    requirePermission(actor, Function::SUBMIT_WORK);
    const std::string cleanName = validate::text(fileName, "The file name", 255);
    if (content.empty()) throw ValidationError("The file is empty.");
    if (content.size() > MAX_BYTES) throw ValidationError("The file is larger than 5 MB.");
    auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment " + std::to_string(assignmentId) + " does not exist.");
    if (!courses_.isEnrolled(assignment->getCourseId(), actor.getId())) {
        throw PermissionError("You are not enrolled in the course of this assignment.");
    }

    const DateTime now = clock_.now();
    const DateTime due = deadlines_.deadlineFor(assignmentId, actor.getId());
    auto existing = submissions_.find(assignmentId, actor.getId());

    SubmissionPreview preview;
    preview.assignmentId = assignmentId;
    preview.assignmentTitle = assignment->getTitle();
    preview.fileName = cleanName;
    preview.content = content;
    preview.deadline = due;
    preview.checkedAt = now;
    preview.maxMark = assignment->getMaxMark();

    if (existing) {
        // BR-03: a Submitted submission can be replaced up to the deadline. Nothing else can.
        if (!existing->canBeReplaced(now, due)) {
            if (existing->getStatus() == SubmissionStatus::LATE) {
                throw RuleError("Late work cannot be replaced.");
            }
            if (existing->getStatus() == SubmissionStatus::SUBMITTED) {
                throw RuleError("The deadline has passed, so the work cannot be replaced.");
            }
            throw RuleError("The work has already been marked and cannot be replaced.");
        }
        auto latest = submissions_.latestVersion(existing->getId());
        preview.isResubmission = true;
        preview.nextVersionNo = (latest ? latest->getVersionNo() : 0) + 1;
        return preview;
    }

    // FR-18: late work is accepted. The student sees the days late and the penalty first.
    const int days = penalty_.daysLate(due, now);
    preview.isLate = days > 0;
    preview.daysLate = days;
    if (days > 0) {
        const PenaltyResult worst = penalty_.apply(preview.maxMark, preview.maxMark, days);
        preview.maxPenaltyMarks = worst.penalty;
        preview.getsZero = worst.finalMark == 0.0;
    }
    return preview;
}

Receipt SubmissionService::confirmSubmission(const User& actor, const SubmissionPreview& preview) {
    // The checks are made again with the time of now. If the deadline passed while the student was
    // reading the preview, the student must see the new penalty first (FR-18).
    const SubmissionPreview fresh = prepareSubmission(actor, preview.assignmentId, preview.fileName, preview.content);
    if (fresh.isLate != preview.isLate || fresh.daysLate != preview.daysLate ||
        fresh.isResubmission != preview.isResubmission) {
        throw RuleError(
            "The situation changed while you were confirming (the deadline passed). "
            "Please submit again and read the new message.");
    }
    const DateTime now = clock_.now();
    Receipt receipt;
    receipt.assignmentTitle = fresh.assignmentTitle;
    receipt.fileName = fresh.fileName;
    receipt.submittedAt = now;

    auto transaction = uow_.begin();
    if (fresh.isResubmission) {
        auto existing = submissions_.find(fresh.assignmentId, actor.getId());
        if (!existing) throw NotFoundError("The earlier submission cannot be found.");
        submissions_.addVersion(existing->getId(), SubmissionVersion(0, existing->getId(), fresh.nextVersionNo,
                                                                     fresh.fileName, fresh.content, now));
        receipt.versionNo = fresh.nextVersionNo;
        receipt.status = existing->getStatus();
        receipt.daysLate = existing->getDaysLate();
    } else {
        const SubmissionStatus status = fresh.isLate ? SubmissionStatus::LATE : SubmissionStatus::SUBMITTED;
        const int id = submissions_.add(Submission(0, fresh.assignmentId, actor.getId(), status, fresh.daysLate));
        submissions_.addVersion(id, SubmissionVersion(0, id, 1, fresh.fileName, fresh.content, now));
        submissions_.addStatusChange(StatusChange(0, id, std::nullopt, status, actor.getId(), now));
        receipt.versionNo = 1;
        receipt.status = status;
        receipt.daysLate = fresh.daysLate;
    }
    transaction->commit();
    return receipt;
}

std::vector<ResultRow> SubmissionService::listMyResults(const User& actor) {
    requirePermission(actor, Function::VIEW_RELEASED_MARKS);
    std::vector<ResultRow> rows;
    for (const Submission& s : submissions_.listByStudent(actor.getId())) {
        if (!s.isVisibleToStudent()) continue;  // BR-02: nothing before the status is Released
        auto assignment = assignments_.findById(s.getAssignmentId());
        if (!assignment) continue;
        auto course = courses_.findCourse(assignment->getCourseId());
        ResultRow row;
        row.courseCode = course ? course->getCode() : "";
        row.assignmentTitle = assignment->getTitle();
        row.maxMark = assignment->getMaxMark();
        row.daysLate = s.getDaysLate();
        row.penalty = s.getPenalty().value_or(0.0);
        row.finalMark = s.getFinalMark().value_or(0.0);
        row.feedback = s.getFeedback().value_or("");
        rows.push_back(row);
    }
    return rows;
}

}  // namespace lms
