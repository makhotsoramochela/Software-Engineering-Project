#include "AdminService.h"

#include <algorithm>
#include <cctype>

#include "Exceptions.h"
#include "Validation.h"

namespace lms {

namespace {
std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}
}  // namespace

AdminService::AdminService(IUserRepository& users, ICourseRepository& courses, PasswordHasher& hasher, UnitOfWork& uow)
    : users_(users), courses_(courses), hasher_(hasher), uow_(uow) {}

Department AdminService::requireDepartment(int departmentId) {
    auto department = courses_.findDepartment(departmentId);
    if (!department) throw NotFoundError("Department " + std::to_string(departmentId) + " does not exist.");
    return *department;
}

Course AdminService::requireCourse(int courseId) {
    auto course = courses_.findCourse(courseId);
    if (!course) throw NotFoundError("Course " + std::to_string(courseId) + " does not exist.");
    return *course;
}

std::unique_ptr<User> AdminService::requireUser(int userId) {
    auto user = users_.findById(userId);
    if (!user) throw NotFoundError("User " + std::to_string(userId) + " does not exist.");
    return user;
}

// FR-04: a department has at most one active Head of Department.
void AdminService::checkHeadOfDepartmentSlot(int departmentId, std::optional<int> exceptUserId) {
    const Department department = requireDepartment(departmentId);
    if (!department.isActive()) throw RuleError("The department " + department.getName() + " is not active.");
    auto head = users_.activeHeadOf(departmentId);
    if (head && (!exceptUserId || head->getId() != *exceptUserId)) {
        throw RuleError("The department " + department.getName() + " already has an active Head of Department (" +
                        head->getUsername() + ").");
    }
}

int AdminService::createFirstAdministrator(const std::string& username, const std::string& fullName,
                                           const std::string& password) {
    if (!users_.list().empty()) throw RuleError("The first Administrator can be created only in an empty system.");
    const std::string name = validate::username(username);
    const std::string full = validate::text(fullName, "The full name", 100);
    validate::password(password);
    auto transaction = uow_.begin();
    const auto user = factory_.create(Role::ADMINISTRATOR, 0, name, full, hasher_.hash(password), std::nullopt);
    const int id = users_.add(*user);
    transaction->commit();
    return id;
}

int AdminService::createUser(const User& actor, const std::string& username, const std::string& fullName,
                             const std::string& password, Role role, std::optional<int> departmentId) {
    requirePermission(actor, Function::MANAGE_USERS);
    const std::string name = validate::username(username);
    const std::string full = validate::text(fullName, "The full name", 100);
    validate::password(password);
    if (users_.findByUsername(name)) throw RuleError("The username " + name + " is already used.");
    if (role == Role::HEAD_OF_DEPARTMENT && departmentId) checkHeadOfDepartmentSlot(*departmentId, std::nullopt);
    const auto user = factory_.create(role, 0, name, full, hasher_.hash(password), departmentId);
    auto transaction = uow_.begin();
    const int id = users_.add(*user);
    transaction->commit();
    return id;
}

void AdminService::editUser(const User& actor, int userId, const std::string& fullName,
                            std::optional<int> departmentId) {
    requirePermission(actor, Function::MANAGE_USERS);
    auto user = requireUser(userId);
    user->setFullName(validate::text(fullName, "The full name", 100));
    if (user->role() == Role::HEAD_OF_DEPARTMENT) {
        if (!departmentId) throw ValidationError("A Head of Department must have a department.");
        if (*departmentId != user->departmentId() && user->isActive()) {
            checkHeadOfDepartmentSlot(*departmentId, user->getId());
        }
        user->setDepartmentId(*departmentId);
    } else if (departmentId) {
        throw ValidationError("Only a Head of Department has a department.");
    }
    auto transaction = uow_.begin();
    users_.update(*user);
    transaction->commit();
}

void AdminService::deactivateUser(const User& actor, int userId) {
    requirePermission(actor, Function::MANAGE_USERS);
    if (actor.getId() == userId) throw RuleError("You cannot deactivate your own account.");
    auto user = requireUser(userId);
    user->setActive(false);
    auto transaction = uow_.begin();
    users_.update(*user);
    transaction->commit();
}

void AdminService::resetPassword(const User& actor, int userId, const std::string& newPassword) {
    requirePermission(actor, Function::MANAGE_USERS);
    validate::password(newPassword);
    auto user = requireUser(userId);
    user->setPasswordHash(hasher_.hash(newPassword));
    user->setFailedLogins(0);  // a new password also opens a locked account
    user->setLockedUntil(std::nullopt);
    auto transaction = uow_.begin();
    users_.update(*user);
    transaction->commit();
}

int AdminService::createDepartment(const User& actor, const std::string& name) {
    requirePermission(actor, Function::MANAGE_COURSES);
    const std::string clean = validate::text(name, "The department name", 100);
    for (const Department& d : courses_.listDepartments()) {
        if (lower(d.getName()) == lower(clean)) throw RuleError("A department with this name already exists.");
    }
    auto transaction = uow_.begin();
    const int id = courses_.addDepartment(Department(0, clean));
    transaction->commit();
    return id;
}

void AdminService::editDepartment(const User& actor, int departmentId, const std::string& name) {
    requirePermission(actor, Function::MANAGE_COURSES);
    Department department = requireDepartment(departmentId);
    const std::string clean = validate::text(name, "The department name", 100);
    for (const Department& d : courses_.listDepartments()) {
        if (d.getId() != departmentId && lower(d.getName()) == lower(clean)) {
            throw RuleError("A department with this name already exists.");
        }
    }
    department.rename(clean);
    auto transaction = uow_.begin();
    courses_.updateDepartment(department);
    transaction->commit();
}

void AdminService::deactivateDepartment(const User& actor, int departmentId) {
    requirePermission(actor, Function::MANAGE_COURSES);
    Department department = requireDepartment(departmentId);
    for (const Course& c : courses_.listCoursesOfDepartment(departmentId)) {
        if (c.isActive()) throw RuleError("Deactivate the active courses of this department first.");
    }
    if (users_.activeHeadOf(departmentId)) {
        throw RuleError("Deactivate or move the Head of Department of this department first.");
    }
    department.setActive(false);
    auto transaction = uow_.begin();
    courses_.updateDepartment(department);
    transaction->commit();
}

int AdminService::createCourse(const User& actor, const std::string& code, const std::string& title, int departmentId) {
    requirePermission(actor, Function::MANAGE_COURSES);
    const std::string cleanCode = validate::text(code, "The course code", 20);
    const std::string cleanTitle = validate::text(title, "The course title", 100);
    const Department department = requireDepartment(departmentId);
    if (!department.isActive()) throw RuleError("The department " + department.getName() + " is not active.");
    for (const Course& c : courses_.listCourses()) {
        if (lower(c.getCode()) == lower(cleanCode))
            throw RuleError("The course code " + cleanCode + " is already used.");
    }
    auto transaction = uow_.begin();
    const int id = courses_.addCourse(Course(0, cleanCode, cleanTitle, departmentId));
    transaction->commit();
    return id;
}

void AdminService::editCourse(const User& actor, int courseId, const std::string& code, const std::string& title) {
    requirePermission(actor, Function::MANAGE_COURSES);
    Course course = requireCourse(courseId);
    const std::string cleanCode = validate::text(code, "The course code", 20);
    const std::string cleanTitle = validate::text(title, "The course title", 100);
    for (const Course& c : courses_.listCourses()) {
        if (c.getId() != courseId && lower(c.getCode()) == lower(cleanCode)) {
            throw RuleError("The course code " + cleanCode + " is already used.");
        }
    }
    course.edit(cleanCode, cleanTitle);
    auto transaction = uow_.begin();
    courses_.updateCourse(course);
    transaction->commit();
}

void AdminService::deactivateCourse(const User& actor, int courseId) {
    requirePermission(actor, Function::MANAGE_COURSES);
    Course course = requireCourse(courseId);
    course.setActive(false);
    auto transaction = uow_.begin();
    courses_.updateCourse(course);
    transaction->commit();
}

void AdminService::assignLecturer(const User& actor, int courseId, int lecturerId) {
    requirePermission(actor, Function::ASSIGN_LECTURERS);
    const Course course = requireCourse(courseId);
    if (!course.isActive()) throw RuleError("The course " + course.getCode() + " is not active.");
    auto lecturer = requireUser(lecturerId);
    if (lecturer->role() != Role::LECTURER || !lecturer->isActive()) {
        throw RuleError("Only an active lecturer can be assigned to a course.");
    }
    if (courses_.lecturerTeaches(courseId, lecturerId)) throw RuleError("This lecturer already teaches the course.");
    auto transaction = uow_.begin();
    courses_.assignLecturer(courseId, lecturerId);
    transaction->commit();
}

void AdminService::enrolStudent(const User& actor, int courseId, int studentId) {
    requirePermission(actor, Function::ENROL_STUDENTS);
    const Course course = requireCourse(courseId);
    if (!course.isActive()) throw RuleError("The course " + course.getCode() + " is not active.");
    auto student = requireUser(studentId);
    if (student->role() != Role::STUDENT || !student->isActive()) {
        throw RuleError("Only an active student can be enrolled in a course.");
    }
    if (courses_.isEnrolled(courseId, studentId)) throw RuleError("This student is already enrolled in the course.");
    auto transaction = uow_.begin();
    courses_.enrol(courseId, studentId);
    transaction->commit();
}

void AdminService::removeStudent(const User& actor, int courseId, int studentId) {
    requirePermission(actor, Function::ENROL_STUDENTS);
    requireCourse(courseId);
    if (!courses_.isEnrolled(courseId, studentId)) throw NotFoundError("This student is not enrolled in the course.");
    auto transaction = uow_.begin();
    courses_.removeEnrolment(courseId, studentId);
    transaction->commit();
}

std::vector<std::unique_ptr<User>> AdminService::listUsers(const User& actor) {
    requirePermission(actor, Function::MANAGE_USERS);
    return users_.list();
}

std::vector<Department> AdminService::listDepartments(const User& actor) {
    requirePermission(actor, Function::MANAGE_COURSES);
    return courses_.listDepartments();
}

std::vector<Course> AdminService::listCourses(const User& actor) {
    requirePermission(actor, Function::MANAGE_COURSES);
    return courses_.listCourses();
}

}  // namespace lms
