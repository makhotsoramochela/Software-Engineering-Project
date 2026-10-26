#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

// AR-9: this class has an operation to add and an operation to read. Nothing else touches audit_entry.
// The schema adds a second barrier: triggers refuse any UPDATE or DELETE of an audit entry.

void SqliteAuditRepository::add(const AuditEntry& e) {
    Statement st = db_.prepare(
        "INSERT INTO audit_entry (submission_id, changed_by, changed_at, old_final_mark, new_final_mark, reason) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    st.bindInt(1, e.getSubmissionId());
    st.bindInt(2, e.getChangedBy());
    st.bindText(3, e.getChangedAt().toString());
    st.bindOptionalDouble(4, e.getOldFinalMark());
    st.bindDouble(5, e.getNewFinalMark());
    st.bindText(6, e.getReason());
    st.step();
}

std::vector<AuditEntry> SqliteAuditRepository::listByDepartment(int departmentId) {
    Statement st = db_.prepare(
        "SELECT e.audit_id, e.submission_id, e.changed_by, e.changed_at, e.old_final_mark, e.new_final_mark, "
        "e.reason, cu.full_name, su.full_name, c.code, a.title "
        "FROM audit_entry e "
        "JOIN submission s ON s.submission_id = e.submission_id "
        "JOIN assignment a ON a.assignment_id = s.assignment_id "
        "JOIN course c ON c.course_id = a.course_id "
        "JOIN user_account cu ON cu.user_id = e.changed_by "
        "JOIN user_account su ON su.user_id = s.student_id "
        "WHERE c.department_id = ? ORDER BY e.audit_id DESC");
    st.bindInt(1, departmentId);
    std::vector<AuditEntry> rows;
    while (st.step()) {
        AuditEntry entry(static_cast<int>(st.columnInt(0)), static_cast<int>(st.columnInt(1)),
                         static_cast<int>(st.columnInt(2)), DateTime::parse(st.columnText(3)),
                         st.columnOptionalDouble(4), st.columnDouble(5), st.columnText(6));
        rows.push_back(entry.withNames(st.columnText(7), st.columnText(8), st.columnText(9), st.columnText(10)));
    }
    return rows;
}

}  // namespace lms
