#include "Exceptions.h"
#include "SqliteRepositories.h"

namespace lms {

namespace {
Department departmentFromRow(const Statement& row) {
    return Department(static_cast<int>(row.columnInt(0)), row.columnText(1), row.columnInt(2) != 0);
}
const char* const COURSE_COLUMNS = "c.course_id, c.code, c.title, c.department_id, c.active";
}  // namespace

std::optional<Department> SqliteCourseRepository::findDepartment(int id) {
    Statement st = db_.prepare("SELECT department_id, name, active FROM department WHERE department_id = ?");
    st.bindInt(1, id);
    if (!st.step()) return std::nullopt;
    return departmentFromRow(st);
}

std::vector<Department> SqliteCourseRepository::listDepartments() {
    Statement st = db_.prepare("SELECT department_id, name, active FROM department ORDER BY department_id");
    std::vector<Department> rows;
    while (st.step()) rows.push_back(departmentFromRow(st));
    return rows;
}

int SqliteCourseRepository::addDepartment(const Department& d) {
    Statement st = db_.prepare("INSERT INTO department (name, active) VALUES (?, ?)");
    st.bindText(1, d.getName());
    st.bindInt(2, d.isActive() ? 1 : 0);
    st.step();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::updateDepartment(const Department& d) {
    Statement st = db_.prepare("UPDATE department SET name = ?, active = ? WHERE department_id = ?");
    st.bindText(1, d.getName());
    st.bindInt(2, d.isActive() ? 1 : 0);
    st.bindInt(3, d.getId());
    st.step();
}

std::vector<Course> SqliteCourseRepository::queryCourses(Statement& st) const {
    std::vector<Course> rows;
    while (st.step()) {
        rows.emplace_back(static_cast<int>(st.columnInt(0)), st.columnText(1), st.columnText(2),
                          static_cast<int>(st.columnInt(3)), st.columnInt(4) != 0);
    }
    return rows;
}

std::optional<Course> SqliteCourseRepository::findCourse(int id) {
    Statement st = db_.prepare(std::string("SELECT ") + COURSE_COLUMNS + " FROM course c WHERE c.course_id = ?");
    st.bindInt(1, id);
    auto rows = queryCourses(st);
    if (rows.empty()) return std::nullopt;
    return rows.front();
}

std::vector<Course> SqliteCourseRepository::listCourses() {
    Statement st = db_.prepare(std::string("SELECT ") + COURSE_COLUMNS + " FROM course c ORDER BY c.code");
    return queryCourses(st);
}

std::vector<Course> SqliteCourseRepository::listCoursesOfDepartment(int departmentId) {
    Statement st = db_.prepare(std::string("SELECT ") + COURSE_COLUMNS +
                               " FROM course c WHERE c.department_id = ? ORDER BY c.code");
    st.bindInt(1, departmentId);
    return queryCourses(st);
}

std::vector<Course> SqliteCourseRepository::listCoursesOfLecturer(int lecturerId) {
    Statement st = db_.prepare(std::string("SELECT ") + COURSE_COLUMNS +
                               " FROM course c JOIN course_lecturer cl ON cl.course_id = c.course_id "
                               "WHERE cl.lecturer_id = ? AND c.active = 1 ORDER BY c.code");
    st.bindInt(1, lecturerId);
    return queryCourses(st);
}

std::vector<Course> SqliteCourseRepository::listCoursesOfStudent(int studentId) {
    Statement st = db_.prepare(std::string("SELECT ") + COURSE_COLUMNS +
                               " FROM course c JOIN enrolment e ON e.course_id = c.course_id "
                               "WHERE e.student_id = ? AND c.active = 1 ORDER BY c.code");
    st.bindInt(1, studentId);
    return queryCourses(st);
}

int SqliteCourseRepository::addCourse(const Course& c) {
    Statement st = db_.prepare("INSERT INTO course (code, title, department_id, active) VALUES (?, ?, ?, ?)");
    st.bindText(1, c.getCode());
    st.bindText(2, c.getTitle());
    st.bindInt(3, c.getDepartmentId());
    st.bindInt(4, c.isActive() ? 1 : 0);
    st.step();
    return static_cast<int>(db_.lastInsertId());
}

void SqliteCourseRepository::updateCourse(const Course& c) {
    Statement st = db_.prepare("UPDATE course SET code = ?, title = ?, active = ? WHERE course_id = ?");
    st.bindText(1, c.getCode());
    st.bindText(2, c.getTitle());
    st.bindInt(3, c.isActive() ? 1 : 0);
    st.bindInt(4, c.getId());
    st.step();
}

void SqliteCourseRepository::assignLecturer(int courseId, int lecturerId) {
    Statement st = db_.prepare("INSERT INTO course_lecturer (course_id, lecturer_id) VALUES (?, ?)");
    st.bindInt(1, courseId);
    st.bindInt(2, lecturerId);
    st.step();
}

bool SqliteCourseRepository::lecturerTeaches(int courseId, int lecturerId) {
    Statement st = db_.prepare("SELECT 1 FROM course_lecturer WHERE course_id = ? AND lecturer_id = ?");
    st.bindInt(1, courseId);
    st.bindInt(2, lecturerId);
    return st.step();
}

void SqliteCourseRepository::enrol(int courseId, int studentId) {
    Statement st = db_.prepare("INSERT INTO enrolment (course_id, student_id) VALUES (?, ?)");
    st.bindInt(1, courseId);
    st.bindInt(2, studentId);
    st.step();
}

void SqliteCourseRepository::removeEnrolment(int courseId, int studentId) {
    Statement st = db_.prepare("DELETE FROM enrolment WHERE course_id = ? AND student_id = ?");
    st.bindInt(1, courseId);
    st.bindInt(2, studentId);
    st.step();
}

bool SqliteCourseRepository::isEnrolled(int courseId, int studentId) {
    Statement st = db_.prepare("SELECT 1 FROM enrolment WHERE course_id = ? AND student_id = ?");
    st.bindInt(1, courseId);
    st.bindInt(2, studentId);
    return st.step();
}

std::vector<int> SqliteCourseRepository::listStudentIds(int courseId) {
    Statement st = db_.prepare("SELECT student_id FROM enrolment WHERE course_id = ? ORDER BY student_id");
    st.bindInt(1, courseId);
    std::vector<int> ids;
    while (st.step()) ids.push_back(static_cast<int>(st.columnInt(0)));
    return ids;
}

}  // namespace lms
