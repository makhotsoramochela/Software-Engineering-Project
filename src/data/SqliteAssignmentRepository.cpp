#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

namespace {
const char* const ASSIGNMENT_COLUMNS = "assignment_id, course_id, title, description, deadline, max_mark";
}  // namespace

Assignment SqliteAssignmentRepository::fromRow(const Statement& row) {
    return Assignment(static_cast<int>(row.columnInt(0)), static_cast<int>(row.columnInt(1)), row.columnText(2),
                      row.columnText(3), DateTime::parse(row.columnText(4)), static_cast<int>(row.columnInt(5)));
}

std::optional<Assignment> SqliteAssignmentRepository::findById(int id) {
    Statement st =
        db_.prepare(std::string("SELECT ") + ASSIGNMENT_COLUMNS + " FROM assignment WHERE assignment_id = ?");
    st.bindInt(1, id);
    if (!st.step()) return std::nullopt;
    return fromRow(st);
}

std::vector<Assignment> SqliteAssignmentRepository::listByCourse(int courseId) {
    Statement st = db_.prepare(std::string("SELECT ") + ASSIGNMENT_COLUMNS +
                               " FROM assignment WHERE course_id = ? ORDER BY deadline, assignment_id");
    st.bindInt(1, courseId);
    std::vector<Assignment> rows;
    while (st.step()) rows.push_back(fromRow(st));
    return rows;
}

int SqliteAssignmentRepository::add(const Assignment& a) {
    Statement st = db_.prepare(
        "INSERT INTO assignment (course_id, title, description, deadline, max_mark) VALUES (?, ?, ?, ?, ?)");
    st.bindInt(1, a.getCourseId());
    st.bindText(2, a.getTitle());
    st.bindText(3, a.getDescription());
    st.bindText(4, a.getDeadline().toString());
    st.bindInt(5, a.getMaxMark());
    st.step();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteAssignmentRepository::addExtension(const Extension& e) {
    Statement st = db_.prepare(
        "INSERT INTO extension (assignment_id, student_id, new_deadline, reason, granted_by, granted_at) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    st.bindInt(1, e.getAssignmentId());
    st.bindInt(2, e.getStudentId());
    st.bindText(3, e.getNewDeadline().toString());
    st.bindText(4, e.getReason());
    st.bindInt(5, e.getGrantedBy());
    st.bindText(6, e.getGrantedAt().toString());
    st.step();
}

std::optional<Extension> SqliteAssignmentRepository::latestExtension(int assignmentId, int studentId) {
    Statement st = db_.prepare(
        "SELECT extension_id, assignment_id, student_id, new_deadline, reason, granted_by, granted_at "
        "FROM extension WHERE assignment_id = ? AND student_id = ? ORDER BY new_deadline DESC, extension_id DESC LIMIT "
        "1");
    st.bindInt(1, assignmentId);
    st.bindInt(2, studentId);
    if (!st.step()) return std::nullopt;
    return Extension(static_cast<int>(st.columnInt(0)), static_cast<int>(st.columnInt(1)),
                     static_cast<int>(st.columnInt(2)), DateTime::parse(st.columnText(3)), st.columnText(4),
                     static_cast<int>(st.columnInt(5)), DateTime::parse(st.columnText(6)));
}

}  // namespace lms
