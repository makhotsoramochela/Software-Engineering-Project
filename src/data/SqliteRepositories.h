// Layer 3 (data access): the SQLite implementations of the repository interfaces of Layer 2
// (design: Repository pattern, Figure 3.10 to 3.12). All SQL of the program is in these classes.
#pragma once

#include "Database.h"
#include "Ports.h"
#include "UserFactory.h"

namespace lms {

class SqliteUserRepository : public IUserRepository {
public:
    explicit SqliteUserRepository(Database& db) : db_(db) {}
    std::unique_ptr<User> findById(int id) override;
    std::unique_ptr<User> findByUsername(const std::string& username) override;
    std::unique_ptr<User> activeHeadOf(int departmentId) override;
    std::vector<std::unique_ptr<User>> list() override;
    int add(const User& user) override;
    void update(const User& user) override;

private:
    std::unique_ptr<User> fromRow(const Statement& row) const;
    Database& db_;
    UserFactory factory_;
};

class SqliteCourseRepository : public ICourseRepository {
public:
    explicit SqliteCourseRepository(Database& db) : db_(db) {}
    std::optional<Department> findDepartment(int id) override;
    std::vector<Department> listDepartments() override;
    int addDepartment(const Department& d) override;
    void updateDepartment(const Department& d) override;
    std::optional<Course> findCourse(int id) override;
    std::vector<Course> listCourses() override;
    std::vector<Course> listCoursesOfDepartment(int departmentId) override;
    std::vector<Course> listCoursesOfLecturer(int lecturerId) override;
    std::vector<Course> listCoursesOfStudent(int studentId) override;
    int addCourse(const Course& c) override;
    void updateCourse(const Course& c) override;
    void assignLecturer(int courseId, int lecturerId) override;
    bool lecturerTeaches(int courseId, int lecturerId) override;
    void enrol(int courseId, int studentId) override;
    void removeEnrolment(int courseId, int studentId) override;
    bool isEnrolled(int courseId, int studentId) override;
    std::vector<int> listStudentIds(int courseId) override;

private:
    std::vector<Course> queryCourses(Statement& st) const;
    Database& db_;
};

class SqliteAssignmentRepository : public IAssignmentRepository {
public:
    explicit SqliteAssignmentRepository(Database& db) : db_(db) {}
    std::optional<Assignment> findById(int id) override;
    std::vector<Assignment> listByCourse(int courseId) override;
    int add(const Assignment& a) override;
    void addExtension(const Extension& e) override;
    std::optional<Extension> latestExtension(int assignmentId, int studentId) override;

private:
    static Assignment fromRow(const Statement& row);
    Database& db_;
};

class SqliteSubmissionRepository : public ISubmissionRepository {
public:
    explicit SqliteSubmissionRepository(Database& db) : db_(db) {}
    std::optional<Submission> findById(int id) override;
    std::optional<Submission> find(int assignmentId, int studentId) override;
    int add(const Submission& s) override;
    void update(const Submission& s) override;
    void addVersion(int submissionId, const SubmissionVersion& v) override;
    std::optional<SubmissionVersion> latestVersion(int submissionId) override;
    std::vector<SubmissionRow> listByAssignment(int assignmentId) override;
    std::vector<Submission> listByStudent(int studentId) override;
    std::vector<SubmissionRow> listByStatus(SubmissionStatus status, int departmentId) override;
    std::map<SubmissionStatus, int> countByStatus(int assignmentId) override;
    void addStatusChange(const StatusChange& c) override;

private:
    static Submission fromRow(const Statement& row);
    std::vector<SubmissionRow> queryRows(Statement& st) const;
    Database& db_;
};

class SqliteAuditRepository : public IAuditRepository {
public:
    explicit SqliteAuditRepository(Database& db) : db_(db) {}
    void add(const AuditEntry& e) override;
    std::vector<AuditEntry> listByDepartment(int departmentId) override;

private:
    Database& db_;
};

class SqliteMessageRepository : public IMessageRepository {
public:
    explicit SqliteMessageRepository(Database& db) : db_(db) {}
    void add(const Message& m) override;
    std::vector<Message> listUnseen(int userId) override;
    void markSeen(int messageId) override;

private:
    Database& db_;
};

}  // namespace lms
