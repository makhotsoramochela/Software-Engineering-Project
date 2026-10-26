// Layer 2 (business logic): the functions of the Administrator (UC-02 to UC-05; FR-04 to FR-08).
// AR-10: a plain-text password is used only here and in AuthService, and goes straight to PasswordHasher.
#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "Ports.h"
#include "UserFactory.h"

namespace lms {

class AdminService {
public:
    AdminService(IUserRepository& users, ICourseRepository& courses, PasswordHasher& hasher, UnitOfWork& uow);

    // First start only: creates the first Administrator when the database has no user at all.
    int createFirstAdministrator(const std::string& username, const std::string& fullName, const std::string& password);

    int createUser(const User& actor, const std::string& username, const std::string& fullName,
                   const std::string& password, Role role, std::optional<int> departmentId);
    void editUser(const User& actor, int userId, const std::string& fullName, std::optional<int> departmentId);
    void deactivateUser(const User& actor, int userId);
    void resetPassword(const User& actor, int userId, const std::string& newPassword);

    int createDepartment(const User& actor, const std::string& name);
    void editDepartment(const User& actor, int departmentId, const std::string& name);
    void deactivateDepartment(const User& actor, int departmentId);

    int createCourse(const User& actor, const std::string& code, const std::string& title, int departmentId);
    void editCourse(const User& actor, int courseId, const std::string& code, const std::string& title);
    void deactivateCourse(const User& actor, int courseId);

    void assignLecturer(const User& actor, int courseId, int lecturerId);
    void enrolStudent(const User& actor, int courseId, int studentId);
    void removeStudent(const User& actor, int courseId, int studentId);

    std::vector<std::unique_ptr<User>> listUsers(const User& actor);
    std::vector<Department> listDepartments(const User& actor);
    std::vector<Course> listCourses(const User& actor);

private:
    Department requireDepartment(int departmentId);
    Course requireCourse(int courseId);
    std::unique_ptr<User> requireUser(int userId);
    void checkHeadOfDepartmentSlot(int departmentId, std::optional<int> exceptUserId);

    IUserRepository& users_;
    ICourseRepository& courses_;
    PasswordHasher& hasher_;
    UnitOfWork& uow_;
    UserFactory factory_;
};

}  // namespace lms
