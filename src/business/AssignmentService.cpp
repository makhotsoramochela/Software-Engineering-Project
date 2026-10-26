#include "AssignmentService.h"

#include "Exceptions.h"
#include "Validation.h"

namespace lms {

AssignmentService::AssignmentService(IAssignmentRepository& assignments, ICourseRepository& courses,
                                     ISubmissionRepository& submissions, IUserRepository& users,
                                     PenaltyStrategy& penalty, Clock& clock, UnitOfWork& uow)
    : assignments_(assignments),
      courses_(courses),
      submissions_(submissions),
      users_(users),
      penalty_(penalty),
      clock_(clock),
      uow_(uow) {}

Assignment AssignmentService::requireAssignmentOfLecturer(const User& actor, int assignmentId) {
    auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment " + std::to_string(assignmentId) + " does not exist.");
    if (!courses_.lecturerTeaches(assignment->getCourseId(), actor.getId())) {
        throw PermissionError("You do not teach the course of this assignment.");
    }
    return *assignment;
}

int AssignmentService::createAssignment(const User& actor, int courseId, const std::string& title,
                                        const std::string& description, const DateTime& deadline, int maxMark) {
    requirePermission(actor, Function::CREATE_ASSIGNMENT);
    auto course = courses_.findCourse(courseId);
    if (!course) throw NotFoundError("Course " + std::to_string(courseId) + " does not exist.");
    if (!courses_.lecturerTeaches(courseId, actor.getId())) throw PermissionError("You do not teach this course.");
    if (!course->isActive()) throw RuleError("The course " + course->getCode() + " is not active.");
    const std::string cleanTitle = validate::text(title, "The title", 100);
    const std::string cleanDescription = validate::text(description, "The description", 2000);
    validate::maxMark(maxMark);
    auto transaction = uow_.begin();
    const int id = assignments_.add(Assignment(0, courseId, cleanTitle, cleanDescription, deadline, maxMark));
    transaction->commit();
    return id;
}

DateTime AssignmentService::deadlineFor(int assignmentId, int studentId) {
    auto assignment = assignments_.findById(assignmentId);
    if (!assignment) throw NotFoundError("Assignment " + std::to_string(assignmentId) + " does not exist.");
    auto extension = assignments_.latestExtension(assignmentId, studentId);
    return extension ? extension->getNewDeadline() : assignment->getDeadline();
}

void AssignmentService::grantExtension(const User& actor, int assignmentId, int studentId, const DateTime& newDeadline,
                                       const std::string& reason) {
    requirePermission(actor, Function::GRANT_EXTENSION);
    const Assignment assignment = requireAssignmentOfLecturer(actor, assignmentId);
    auto student = users_.findById(studentId);
    if (!student || student->role() != Role::STUDENT) throw NotFoundError("Student does not exist.");
    if (!courses_.isEnrolled(assignment.getCourseId(), studentId)) {
        throw RuleError("This student is not enrolled in the course of the assignment.");
    }
    const std::string cleanReason = validate::text(reason, "The reason", 500);
    const DateTime current = deadlineFor(assignmentId, studentId);
    if (!(newDeadline > current)) {
        throw RuleError("The new deadline must be later than the current deadline of the student (" +
                        current.toDisplay() + ").");
    }
    auto submission = submissions_.find(assignmentId, studentId);
    if (submission && submission->getStatus() != SubmissionStatus::SUBMITTED &&
        submission->getStatus() != SubmissionStatus::LATE) {
        throw RuleError("An extension cannot be given after the work has been marked.");
    }

    auto transaction = uow_.begin();
    const DateTime now = clock_.now();
    assignments_.addExtension(Extension(0, assignmentId, studentId, newDeadline, cleanReason, actor.getId(), now));
    if (submission && submission->getStatus() == SubmissionStatus::LATE) {
        // BR-04: work out n again with the new deadline.
        auto version = submissions_.latestVersion(submission->getId());
        const int days = version ? penalty_.daysLate(newDeadline, version->getSubmittedAt()) : 0;
        if (days == 0) {
            submission->changeStatus(SubmissionStatus::SUBMITTED);
            submission->setDaysLate(0);
            submissions_.addStatusChange(StatusChange(0, submission->getId(), SubmissionStatus::LATE,
                                                      SubmissionStatus::SUBMITTED, actor.getId(), now,
                                                      "Extension granted: " + cleanReason));
        } else {
            submission->setDaysLate(days);
        }
        submissions_.update(*submission);
    }
    transaction->commit();
}

std::vector<AssignmentInfo> AssignmentService::listMine(const User& actor) {
    requirePermission(actor, Function::CREATE_ASSIGNMENT);
    std::vector<AssignmentInfo> rows;
    for (const Course& course : courses_.listCoursesOfLecturer(actor.getId())) {
        for (const Assignment& a : assignments_.listByCourse(course.getId())) {
            AssignmentInfo info;
            info.assignmentId = a.getId();
            info.courseCode = course.getCode();
            info.courseTitle = course.getTitle();
            info.title = a.getTitle();
            info.deadline = a.getDeadline();
            info.maxMark = a.getMaxMark();
            rows.push_back(info);
        }
    }
    return rows;
}

std::vector<Course> AssignmentService::listMyCourses(const User& actor) {
    requirePermission(actor, Function::VIEW_COURSES);
    if (actor.role() == Role::STUDENT) return courses_.listCoursesOfStudent(actor.getId());
    return courses_.listCoursesOfLecturer(actor.getId());
}

std::vector<std::unique_ptr<User>> AssignmentService::listEnrolledStudents(const User& actor, int assignmentId) {
    requirePermission(actor, Function::GRANT_EXTENSION);
    const Assignment assignment = requireAssignmentOfLecturer(actor, assignmentId);
    std::vector<std::unique_ptr<User>> students;
    for (const int id : courses_.listStudentIds(assignment.getCourseId())) {
        if (auto student = users_.findById(id)) students.push_back(std::move(student));
    }
    return students;
}

}  // namespace lms
