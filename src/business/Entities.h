// Layer 2 (business logic): the domain classes other than User (design: Figures 3.2 and 3.3).
// Every field is private and has a getter. Only Submission has behaviour.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "DateTime.h"
#include "Types.h"

namespace lms {

class Department {
public:
    Department(int id, std::string name, bool active = true) : id_(id), name_(std::move(name)), active_(active) {}
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    bool isActive() const { return active_; }
    void rename(std::string name) { name_ = std::move(name); }
    void setActive(bool active) { active_ = active; }

private:
    int id_;
    std::string name_;
    bool active_;
};

class Course {
public:
    Course(int id, std::string code, std::string title, int departmentId, bool active = true)
        : id_(id), code_(std::move(code)), title_(std::move(title)), departmentId_(departmentId), active_(active) {}
    int getId() const { return id_; }
    const std::string& getCode() const { return code_; }
    const std::string& getTitle() const { return title_; }
    int getDepartmentId() const { return departmentId_; }
    bool isActive() const { return active_; }
    void edit(std::string code, std::string title) {
        code_ = std::move(code);
        title_ = std::move(title);
    }
    void setActive(bool active) { active_ = active; }

private:
    int id_;
    std::string code_;
    std::string title_;
    int departmentId_;
    bool active_;
};

class Assignment {
public:
    Assignment(int id, int courseId, std::string title, std::string description, DateTime deadline, int maxMark)
        : id_(id),
          courseId_(courseId),
          title_(std::move(title)),
          description_(std::move(description)),
          deadline_(deadline),
          maxMark_(maxMark) {}
    int getId() const { return id_; }
    int getCourseId() const { return courseId_; }
    const std::string& getTitle() const { return title_; }
    const std::string& getDescription() const { return description_; }
    const DateTime& getDeadline() const { return deadline_; }
    int getMaxMark() const { return maxMark_; }

private:
    int id_;
    int courseId_;
    std::string title_;
    std::string description_;
    DateTime deadline_;
    int maxMark_;
};

class Extension {
public:
    Extension(int id, int assignmentId, int studentId, DateTime newDeadline, std::string reason, int grantedBy,
              DateTime grantedAt)
        : id_(id),
          assignmentId_(assignmentId),
          studentId_(studentId),
          newDeadline_(newDeadline),
          reason_(std::move(reason)),
          grantedBy_(grantedBy),
          grantedAt_(grantedAt) {}
    int getId() const { return id_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    const DateTime& getNewDeadline() const { return newDeadline_; }
    const std::string& getReason() const { return reason_; }
    int getGrantedBy() const { return grantedBy_; }
    const DateTime& getGrantedAt() const { return grantedAt_; }

private:
    int id_;
    int assignmentId_;
    int studentId_;
    DateTime newDeadline_;
    std::string reason_;
    int grantedBy_;
    DateTime grantedAt_;
};

class SubmissionVersion {
public:
    SubmissionVersion(int id, int submissionId, int versionNo, std::string fileName, std::vector<std::uint8_t> content,
                      DateTime submittedAt)
        : id_(id),
          submissionId_(submissionId),
          versionNo_(versionNo),
          fileName_(std::move(fileName)),
          content_(std::move(content)),
          submittedAt_(submittedAt) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    int getVersionNo() const { return versionNo_; }
    const std::string& getFileName() const { return fileName_; }
    const std::vector<std::uint8_t>& getContent() const { return content_; }
    const DateTime& getSubmittedAt() const { return submittedAt_; }

private:
    int id_;
    int submissionId_;
    int versionNo_;
    std::string fileName_;
    std::vector<std::uint8_t> content_;
    DateTime submittedAt_;
};

class StatusChange {
public:
    StatusChange(int id, int submissionId, std::optional<SubmissionStatus> from, SubmissionStatus to, int changedBy,
                 DateTime changedAt, std::optional<std::string> comment = std::nullopt)
        : id_(id),
          submissionId_(submissionId),
          from_(from),
          to_(to),
          changedBy_(changedBy),
          changedAt_(changedAt),
          comment_(std::move(comment)) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    const std::optional<SubmissionStatus>& getFrom() const { return from_; }
    SubmissionStatus getTo() const { return to_; }
    int getChangedBy() const { return changedBy_; }
    const DateTime& getChangedAt() const { return changedAt_; }
    const std::optional<std::string>& getComment() const { return comment_; }

private:
    int id_;
    int submissionId_;
    std::optional<SubmissionStatus> from_;
    SubmissionStatus to_;
    int changedBy_;
    DateTime changedAt_;
    std::optional<std::string> comment_;
};

// An entry of the audit log. It has getters only (no setters): it can never be changed (NFR-09).
class AuditEntry {
public:
    AuditEntry(int id, int submissionId, int changedBy, DateTime changedAt, std::optional<double> oldFinalMark,
               double newFinalMark, std::string reason)
        : id_(id),
          submissionId_(submissionId),
          changedBy_(changedBy),
          changedAt_(changedAt),
          oldFinalMark_(oldFinalMark),
          newFinalMark_(newFinalMark),
          reason_(std::move(reason)) {}
    int getId() const { return id_; }
    int getSubmissionId() const { return submissionId_; }
    int getChangedBy() const { return changedBy_; }
    const DateTime& getChangedAt() const { return changedAt_; }
    const std::optional<double>& getOldFinalMark() const { return oldFinalMark_; }
    double getNewFinalMark() const { return newFinalMark_; }
    const std::string& getReason() const { return reason_; }

    // Names for the screen. The repository fills them when it reads the log; they are not stored.
    AuditEntry withNames(std::string changedByName, std::string studentName, std::string courseCode,
                         std::string assignmentTitle) const {
        AuditEntry copy = *this;
        copy.changedByName_ = std::move(changedByName);
        copy.studentName_ = std::move(studentName);
        copy.courseCode_ = std::move(courseCode);
        copy.assignmentTitle_ = std::move(assignmentTitle);
        return copy;
    }
    const std::string& getChangedByName() const { return changedByName_; }
    const std::string& getStudentName() const { return studentName_; }
    const std::string& getCourseCode() const { return courseCode_; }
    const std::string& getAssignmentTitle() const { return assignmentTitle_; }

private:
    int id_;
    int submissionId_;
    int changedBy_;
    DateTime changedAt_;
    std::optional<double> oldFinalMark_;
    double newFinalMark_;
    std::string reason_;
    std::string changedByName_;
    std::string studentName_;
    std::string courseCode_;
    std::string assignmentTitle_;
};

class Message {
public:
    Message(int id, int userId, std::string body, DateTime createdAt, bool seen = false)
        : id_(id), userId_(userId), body_(std::move(body)), createdAt_(createdAt), seen_(seen) {}
    int getId() const { return id_; }
    int getUserId() const { return userId_; }
    const std::string& getBody() const { return body_; }
    const DateTime& getCreatedAt() const { return createdAt_; }
    bool isSeen() const { return seen_; }

private:
    int id_;
    int userId_;
    std::string body_;
    DateTime createdAt_;
    bool seen_;
};

// The submission of one student for one assignment, with its mark (design: Figure 3.3).
class Submission {
public:
    Submission(int id, int assignmentId, int studentId, SubmissionStatus status, int daysLate = 0)
        : id_(id), assignmentId_(assignmentId), studentId_(studentId), status_(status), daysLate_(daysLate) {}

    int getId() const { return id_; }
    int getAssignmentId() const { return assignmentId_; }
    int getStudentId() const { return studentId_; }
    SubmissionStatus getStatus() const { return status_; }
    int getDaysLate() const { return daysLate_; }
    const std::optional<double>& getRawMark() const { return rawMark_; }
    const std::optional<double>& getPenalty() const { return penalty_; }
    const std::optional<double>& getFinalMark() const { return finalMark_; }
    const std::optional<std::string>& getFeedback() const { return feedback_; }
    const std::optional<int>& getMarkedBy() const { return markedBy_; }

    void setDaysLate(int daysLate) { daysLate_ = daysLate; }
    // Used by the repository when it loads a submission from the database.
    void restoreMark(std::optional<double> raw, std::optional<double> penalty, std::optional<double> final,
                     std::optional<std::string> feedback, std::optional<int> markedBy);
    // Saves the mark of the lecturer. The status is changed separately by changeStatus.
    void setMark(double raw, double penalty, double final, std::string feedback, int markedBy);
    // Moderation: the Head of Department changes the final mark. Raw mark and penalty stay.
    void setFinalMark(double final) { finalMark_ = final; }

    // BR-02: what the student may see. Until the status is Released the student sees
    // Submitted (or Late if the work was late), whatever the real status is.
    SubmissionStatus visibleStatus() const;
    bool isVisibleToStudent() const { return status_ == SubmissionStatus::RELEASED; }
    // BR-03: only a Submitted submission can be replaced, and only up to the deadline of the student.
    bool canBeReplaced(const DateTime& now, const DateTime& due) const;
    // FR-24 and Table 9: allows only the changes of the SRS. Throws RuleError for any other change.
    void changeStatus(SubmissionStatus to);

    static bool isAllowedChange(SubmissionStatus from, SubmissionStatus to);

private:
    int id_;
    int assignmentId_;
    int studentId_;
    SubmissionStatus status_;
    int daysLate_;
    std::optional<double> rawMark_;
    std::optional<double> penalty_;
    std::optional<double> finalMark_;
    std::optional<std::string> feedback_;
    std::optional<int> markedBy_;
};

}  // namespace lms
