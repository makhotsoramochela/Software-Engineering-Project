// Layer 2 (business logic): assignments and extensions (UC-06, UC-07; FR-09, FR-10; BR-04).
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "PenaltyStrategy.h"
#include "Ports.h"

namespace lms {

class AssignmentService {
public:
    AssignmentService(IAssignmentRepository& assignments, ICourseRepository& courses,
                      ISubmissionRepository& submissions, IUserRepository& users, PenaltyStrategy& penalty,
                      Clock& clock, UnitOfWork& uow);

    int createAssignment(const User& actor, int courseId, const std::string& title, const std::string& description,
                         const DateTime& deadline, int maxMark);

    // BR-04. If the student already submitted late, n is worked out again with the new deadline.
    void grantExtension(const User& actor, int assignmentId, int studentId, const DateTime& newDeadline,
                        const std::string& reason);

    // The deadline of one student: the assignment deadline, or the latest extension of the student.
    DateTime deadlineFor(int assignmentId, int studentId);

    std::vector<AssignmentInfo> listMine(const User& actor);
    std::vector<Course> listMyCourses(const User& actor);
    std::vector<std::unique_ptr<User>> listEnrolledStudents(const User& actor, int assignmentId);

private:
    Assignment requireAssignmentOfLecturer(const User& actor, int assignmentId);

    IAssignmentRepository& assignments_;
    ICourseRepository& courses_;
    ISubmissionRepository& submissions_;
    IUserRepository& users_;
    PenaltyStrategy& penalty_;
    Clock& clock_;
    UnitOfWork& uow_;
};

}  // namespace lms
