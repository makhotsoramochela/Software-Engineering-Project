// Layer 2 (business logic): the interfaces ("ports") that Layer 3 implements (rule AR-2).
// No file of Layer 2 includes sqlite3.h or sodium.h.
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "DateTime.h"
#include "Dto.h"
#include "Entities.h"
#include "User.h"

namespace lms {

// AR-7: services read the time only through this interface.
class Clock {
public:
    virtual ~Clock() = default;
    virtual DateTime now() const = 0;
};

// AR-10: a plain-text password is handed to this interface and never kept.
class PasswordHasher {
public:
    virtual ~PasswordHasher() = default;
    virtual std::string hash(const std::string& password) const = 0;
    virtual bool verify(const std::string& hash, const std::string& password) const = 0;
};

// AR-5: one transaction for each action that changes data. If the object is destroyed
// before commit() was called, the changes are rolled back.
class Transaction {
public:
    virtual ~Transaction() = default;
    virtual void commit() = 0;
};

class UnitOfWork {
public:
    virtual ~UnitOfWork() = default;
    virtual std::unique_ptr<Transaction> begin() = 0;
};

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual std::unique_ptr<User> findById(int id) = 0;
    virtual std::unique_ptr<User> findByUsername(const std::string& username) = 0;
    virtual std::unique_ptr<User> activeHeadOf(int departmentId) = 0;
    virtual std::vector<std::unique_ptr<User>> list() = 0;
    virtual int add(const User& user) = 0;
    virtual void update(const User& user) = 0;
};

class ICourseRepository {
public:
    virtual ~ICourseRepository() = default;
    virtual std::optional<Department> findDepartment(int id) = 0;
    virtual std::vector<Department> listDepartments() = 0;
    virtual int addDepartment(const Department& d) = 0;
    virtual void updateDepartment(const Department& d) = 0;
    virtual std::optional<Course> findCourse(int id) = 0;
    virtual std::vector<Course> listCourses() = 0;
    virtual std::vector<Course> listCoursesOfDepartment(int departmentId) = 0;
    virtual std::vector<Course> listCoursesOfLecturer(int lecturerId) = 0;
    virtual std::vector<Course> listCoursesOfStudent(int studentId) = 0;
    virtual int addCourse(const Course& c) = 0;
    virtual void updateCourse(const Course& c) = 0;
    virtual void assignLecturer(int courseId, int lecturerId) = 0;
    virtual bool lecturerTeaches(int courseId, int lecturerId) = 0;
    virtual void enrol(int courseId, int studentId) = 0;
    virtual void removeEnrolment(int courseId, int studentId) = 0;
    virtual bool isEnrolled(int courseId, int studentId) = 0;
    virtual std::vector<int> listStudentIds(int courseId) = 0;
};

class IAssignmentRepository {
public:
    virtual ~IAssignmentRepository() = default;
    virtual std::optional<Assignment> findById(int id) = 0;
    virtual std::vector<Assignment> listByCourse(int courseId) = 0;
    virtual int add(const Assignment& a) = 0;
    virtual void addExtension(const Extension& e) = 0;
    virtual std::optional<Extension> latestExtension(int assignmentId, int studentId) = 0;
};

class ISubmissionRepository {
public:
    virtual ~ISubmissionRepository() = default;
    virtual std::optional<Submission> findById(int id) = 0;
    virtual std::optional<Submission> find(int assignmentId, int studentId) = 0;
    virtual int add(const Submission& s) = 0;
    virtual void update(const Submission& s) = 0;
    virtual void addVersion(int submissionId, const SubmissionVersion& v) = 0;
    virtual std::optional<SubmissionVersion> latestVersion(int submissionId) = 0;
    virtual std::vector<SubmissionRow> listByAssignment(int assignmentId) = 0;
    virtual std::vector<Submission> listByStudent(int studentId) = 0;
    virtual std::vector<SubmissionRow> listByStatus(SubmissionStatus status, int departmentId) = 0;
    virtual std::map<SubmissionStatus, int> countByStatus(int assignmentId) = 0;
    virtual void addStatusChange(const StatusChange& c) = 0;
};

// AR-9: only operations to add and to read. There is no operation to change or delete an entry.
class IAuditRepository {
public:
    virtual ~IAuditRepository() = default;
    virtual void add(const AuditEntry& e) = 0;
    virtual std::vector<AuditEntry> listByDepartment(int departmentId) = 0;
};

class IMessageRepository {
public:
    virtual ~IMessageRepository() = default;
    virtual void add(const Message& m) = 0;
    virtual std::vector<Message> listUnseen(int userId) = 0;
    virtual void markSeen(int messageId) = 0;
};

}  // namespace lms
