#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

namespace {
const char* const SUBMISSION_COLUMNS =
    "submission_id, assignment_id, student_id, status, days_late, raw_mark, penalty, final_mark, feedback, marked_by";

// A row for the lists of the lecturer and of the Head of Department. The file itself is not read here.
const char* const ROW_SELECT =
    "SELECT s.submission_id, s.assignment_id, c.code, a.title, a.max_mark, s.student_id, u.full_name, u.username, "
    "s.status, s.days_late, v.submitted_at, "
    "(SELECT COUNT(*) FROM submission_version x WHERE x.submission_id = s.submission_id), "
    "s.raw_mark, s.penalty, s.final_mark, s.feedback, m.full_name "
    "FROM submission s "
    "JOIN assignment a ON a.assignment_id = s.assignment_id "
    "JOIN course c ON c.course_id = a.course_id "
    "JOIN user_account u ON u.user_id = s.student_id "
    "JOIN submission_version v ON v.submission_id = s.submission_id AND v.version_no = "
    "(SELECT MAX(y.version_no) FROM submission_version y WHERE y.submission_id = s.submission_id) "
    "LEFT JOIN user_account m ON m.user_id = s.marked_by ";
}  // namespace

Submission SqliteSubmissionRepository::fromRow(const Statement& row) {
    Submission s(static_cast<int>(row.columnInt(0)), static_cast<int>(row.columnInt(1)),
                 static_cast<int>(row.columnInt(2)), parseStatus(row.columnText(3)),
                 static_cast<int>(row.columnInt(4)));
    s.restoreMark(row.columnOptionalDouble(5), row.columnOptionalDouble(6), row.columnOptionalDouble(7),
                  row.columnOptionalText(8), row.columnOptionalInt(9));
    return s;
}

std::optional<Submission> SqliteSubmissionRepository::findById(int id) {
    Statement st =
        db_.prepare(std::string("SELECT ") + SUBMISSION_COLUMNS + " FROM submission WHERE submission_id = ?");
    st.bindInt(1, id);
    if (!st.step()) return std::nullopt;
    return fromRow(st);
}

std::optional<Submission> SqliteSubmissionRepository::find(int assignmentId, int studentId) {
    Statement st = db_.prepare(std::string("SELECT ") + SUBMISSION_COLUMNS +
                               " FROM submission WHERE assignment_id = ? AND student_id = ?");
    st.bindInt(1, assignmentId);
    st.bindInt(2, studentId);
    if (!st.step()) return std::nullopt;
    return fromRow(st);
}

int SqliteSubmissionRepository::add(const Submission& s) {
    Statement st = db_.prepare(
        "INSERT INTO submission (assignment_id, student_id, status, days_late, raw_mark, penalty, final_mark, "
        "feedback, marked_by) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    st.bindInt(1, s.getAssignmentId());
    st.bindInt(2, s.getStudentId());
    st.bindText(3, toString(s.getStatus()));
    st.bindInt(4, s.getDaysLate());
    st.bindOptionalDouble(5, s.getRawMark());
    st.bindOptionalDouble(6, s.getPenalty());
    st.bindOptionalDouble(7, s.getFinalMark());
    st.bindOptionalText(8, s.getFeedback());
    st.bindOptionalInt(9, s.getMarkedBy());
    st.step();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteSubmissionRepository::update(const Submission& s) {
    Statement st = db_.prepare(
        "UPDATE submission SET status = ?, days_late = ?, raw_mark = ?, penalty = ?, final_mark = ?, feedback = ?, "
        "marked_by = ? WHERE submission_id = ?");
    st.bindText(1, toString(s.getStatus()));
    st.bindInt(2, s.getDaysLate());
    st.bindOptionalDouble(3, s.getRawMark());
    st.bindOptionalDouble(4, s.getPenalty());
    st.bindOptionalDouble(5, s.getFinalMark());
    st.bindOptionalText(6, s.getFeedback());
    st.bindOptionalInt(7, s.getMarkedBy());
    st.bindInt(8, s.getId());
    st.step();
}

void SqliteSubmissionRepository::addVersion(int submissionId, const SubmissionVersion& v) {
    Statement st = db_.prepare(
        "INSERT INTO submission_version (submission_id, version_no, file_name, file_content, submitted_at) "
        "VALUES (?, ?, ?, ?, ?)");
    st.bindInt(1, submissionId);
    st.bindInt(2, v.getVersionNo());
    st.bindText(3, v.getFileName());
    st.bindBlob(4, v.getContent());
    st.bindText(5, v.getSubmittedAt().toString());
    st.step();
}

std::optional<SubmissionVersion> SqliteSubmissionRepository::latestVersion(int submissionId) {
    Statement st = db_.prepare(
        "SELECT version_id, submission_id, version_no, file_name, file_content, submitted_at FROM submission_version "
        "WHERE submission_id = ? ORDER BY version_no DESC LIMIT 1");
    st.bindInt(1, submissionId);
    if (!st.step()) return std::nullopt;
    return SubmissionVersion(static_cast<int>(st.columnInt(0)), static_cast<int>(st.columnInt(1)),
                             static_cast<int>(st.columnInt(2)), st.columnText(3), st.columnBlob(4),
                             DateTime::parse(st.columnText(5)));
}

std::vector<SubmissionRow> SqliteSubmissionRepository::queryRows(Statement& st) const {
    std::vector<SubmissionRow> rows;
    while (st.step()) {
        SubmissionRow r;
        r.submissionId = static_cast<int>(st.columnInt(0));
        r.assignmentId = static_cast<int>(st.columnInt(1));
        r.courseCode = st.columnText(2);
        r.assignmentTitle = st.columnText(3);
        r.maxMark = static_cast<int>(st.columnInt(4));
        r.studentId = static_cast<int>(st.columnInt(5));
        r.studentName = st.columnText(6);
        r.studentUsername = st.columnText(7);
        r.status = parseStatus(st.columnText(8));
        r.daysLate = static_cast<int>(st.columnInt(9));
        r.submittedAt = DateTime::parse(st.columnText(10));
        r.versionCount = static_cast<int>(st.columnInt(11));
        r.rawMark = st.columnOptionalDouble(12);
        r.penalty = st.columnOptionalDouble(13);
        r.finalMark = st.columnOptionalDouble(14);
        r.feedback = st.columnOptionalText(15);
        r.markedByName = st.columnText(16);
        rows.push_back(std::move(r));
    }
    return rows;
}

std::vector<SubmissionRow> SqliteSubmissionRepository::listByAssignment(int assignmentId) {
    Statement st =
        db_.prepare(std::string(ROW_SELECT) + "WHERE s.assignment_id = ? ORDER BY u.full_name, s.submission_id");
    st.bindInt(1, assignmentId);
    return queryRows(st);
}

std::vector<Submission> SqliteSubmissionRepository::listByStudent(int studentId) {
    Statement st = db_.prepare(std::string("SELECT ") + SUBMISSION_COLUMNS +
                               " FROM submission WHERE student_id = ? ORDER BY submission_id");
    st.bindInt(1, studentId);
    std::vector<Submission> rows;
    while (st.step()) rows.push_back(fromRow(st));
    return rows;
}

std::vector<SubmissionRow> SqliteSubmissionRepository::listByStatus(SubmissionStatus status, int departmentId) {
    Statement st = db_.prepare(std::string(ROW_SELECT) +
                               "WHERE s.status = ? AND c.department_id = ? ORDER BY c.code, a.title, u.full_name");
    st.bindText(1, toString(status));
    st.bindInt(2, departmentId);
    return queryRows(st);
}

std::map<SubmissionStatus, int> SqliteSubmissionRepository::countByStatus(int assignmentId) {
    Statement st = db_.prepare("SELECT status, COUNT(*) FROM submission WHERE assignment_id = ? GROUP BY status");
    st.bindInt(1, assignmentId);
    std::map<SubmissionStatus, int> counts;
    while (st.step()) counts[parseStatus(st.columnText(0))] = static_cast<int>(st.columnInt(1));
    return counts;
}

void SqliteSubmissionRepository::addStatusChange(const StatusChange& c) {
    Statement st = db_.prepare(
        "INSERT INTO status_change (submission_id, from_status, to_status, changed_by, changed_at, comment) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    st.bindInt(1, c.getSubmissionId());
    const std::optional<SubmissionStatus> from = c.getFrom();
    st.bindOptionalText(2, from ? std::optional<std::string>(toString(*from)) : std::nullopt);
    st.bindText(3, toString(c.getTo()));
    st.bindInt(4, c.getChangedBy());
    st.bindText(5, c.getChangedAt().toString());
    st.bindOptionalText(6, c.getComment());
    st.step();
}

}  // namespace lms
