#include "AdministratorMenu.h"

#include <map>

#include "Format.h"

namespace lms {

AdministratorMenu::AdministratorMenu(InputReader& input, std::ostream& out, User& user, AdminService& admin)
    : Menu(input, out, 16), user_(user), admin_(admin) {}

void AdministratorMenu::printChoices() {
    out_ << "=== Administrator: " << user_.getFullName() << " ===\n"
         << " 1. List users              9. Deactivate a department\n"
         << " 2. Create a user          10. List courses\n"
         << " 3. Edit a user            11. Create a course\n"
         << " 4. Deactivate a user      12. Edit a course\n"
         << " 5. Set a new password     13. Deactivate a course\n"
         << " 6. List departments       14. Assign a lecturer to a course\n"
         << " 7. Create a department    15. Enrol a student in a course\n"
         << " 8. Edit a department      16. Remove a student from a course\n"
         << " 0. Log out\n";
}

void AdministratorMenu::listUsers() {
    std::map<int, std::string> departmentNames;
    for (const Department& d : admin_.listDepartments(user_)) departmentNames[d.getId()] = d.getName();
    out_ << format::cell("Id", 5) << format::cell("Username", 18) << format::cell("Full name", 26)
         << format::cell("Role", 20) << format::cell("Department", 20) << "Active\n";
    for (const auto& u : admin_.listUsers(user_)) {
        std::string department = "-";
        const std::optional<int> departmentId = u->departmentId();
        if (departmentId) department = departmentNames[*departmentId];
        out_ << format::cell(std::to_string(u->getId()), 5) << format::cell(u->getUsername(), 18)
             << format::cell(u->getFullName(), 26) << format::cell(displayName(u->role()), 20)
             << format::cell(department, 20) << (u->isActive() ? "yes" : "no") << '\n';
    }
}

void AdministratorMenu::listDepartments() {
    out_ << format::cell("Id", 5) << format::cell("Name", 36) << "Active\n";
    for (const Department& d : admin_.listDepartments(user_)) {
        out_ << format::cell(std::to_string(d.getId()), 5) << format::cell(d.getName(), 36)
             << (d.isActive() ? "yes" : "no") << '\n';
    }
}

void AdministratorMenu::listCourses() {
    out_ << format::cell("Id", 5) << format::cell("Code", 12) << format::cell("Title", 34) << format::cell("Dept", 6)
         << "Active\n";
    for (const Course& c : admin_.listCourses(user_)) {
        out_ << format::cell(std::to_string(c.getId()), 5) << format::cell(c.getCode(), 12)
             << format::cell(c.getTitle(), 34) << format::cell(std::to_string(c.getDepartmentId()), 6)
             << (c.isActive() ? "yes" : "no") << '\n';
    }
}

void AdministratorMenu::createUser() {
    const std::string username = input_.readLine("Username");
    const std::string fullName = input_.readLine("Full name");
    const std::string password = input_.readPassword("Password (at least 8 characters, a letter and a digit)");
    const std::string again = input_.readPassword("Type the password again");
    if (password != again) throw ValidationError("The two passwords are not the same.");
    out_ << "Roles: 1 Administrator, 2 Lecturer, 3 Head of Department, 4 Student\n";
    const int roleChoice = input_.readInt("Role", 1, 4);
    static const Role roles[] = {Role::ADMINISTRATOR, Role::LECTURER, Role::HEAD_OF_DEPARTMENT, Role::STUDENT};
    const Role role = roles[roleChoice - 1];
    std::optional<int> departmentId;
    if (role == Role::HEAD_OF_DEPARTMENT) {
        listDepartments();
        departmentId = input_.readInt("Department id", 1, 1000000);
    }
    const int id = admin_.createUser(user_, username, fullName, password, role, departmentId);
    out_ << "User created. The id is " << id << ".\n";
}

void AdministratorMenu::editUser() {
    listUsers();
    const int id = readId("Id of the user");
    // Only a Head of Department has a department, so only then do we ask for a new one.
    std::optional<int> departmentId;
    for (const auto& u : admin_.listUsers(user_)) {
        if (u->getId() == id && u->departmentId()) {
            listDepartments();
            departmentId = input_.readInt("New department id", 1, 1000000);
        }
    }
    const std::string fullName = input_.readLine("New full name");
    admin_.editUser(user_, id, fullName, departmentId);
    out_ << "The user was changed.\n";
}

void AdministratorMenu::deactivateUser() {
    listUsers();
    const int id = readId("Id of the user to deactivate");
    if (!input_.confirm("Deactivate user " + std::to_string(id) + "?")) return;
    admin_.deactivateUser(user_, id);
    out_ << "The user was deactivated.\n";
}

void AdministratorMenu::resetPassword() {
    listUsers();
    const int id = readId("Id of the user");
    const std::string password = input_.readPassword("New password");
    const std::string again = input_.readPassword("Type the new password again");
    if (password != again) throw ValidationError("The two passwords are not the same.");
    admin_.resetPassword(user_, id, password);
    out_ << "The new password was saved.\n";
}

void AdministratorMenu::createDepartment() {
    const std::string name = input_.readLine("Department name");
    out_ << "Department created. The id is " << admin_.createDepartment(user_, name) << ".\n";
}

void AdministratorMenu::editDepartment() {
    listDepartments();
    const int id = readId("Id of the department");
    const std::string name = input_.readLine("New name");
    admin_.editDepartment(user_, id, name);
    out_ << "The department was changed.\n";
}

void AdministratorMenu::deactivateDepartment() {
    listDepartments();
    const int id = readId("Id of the department to deactivate");
    if (!input_.confirm("Deactivate department " + std::to_string(id) + "?")) return;
    admin_.deactivateDepartment(user_, id);
    out_ << "The department was deactivated.\n";
}

void AdministratorMenu::createCourse() {
    listDepartments();
    const int departmentId = readId("Department id");
    const std::string code = input_.readLine("Course code");
    const std::string title = input_.readLine("Course title");
    out_ << "Course created. The id is " << admin_.createCourse(user_, code, title, departmentId) << ".\n";
}

void AdministratorMenu::editCourse() {
    listCourses();
    const int id = readId("Id of the course");
    const std::string code = input_.readLine("New course code");
    const std::string title = input_.readLine("New course title");
    admin_.editCourse(user_, id, code, title);
    out_ << "The course was changed.\n";
}

void AdministratorMenu::deactivateCourse() {
    listCourses();
    const int id = readId("Id of the course to deactivate");
    if (!input_.confirm("Deactivate course " + std::to_string(id) + "?")) return;
    admin_.deactivateCourse(user_, id);
    out_ << "The course was deactivated.\n";
}

void AdministratorMenu::assignLecturer() {
    listCourses();
    const int courseId = readId("Id of the course");
    listUsers();
    const int lecturerId = readId("Id of the lecturer");
    admin_.assignLecturer(user_, courseId, lecturerId);
    out_ << "The lecturer was assigned to the course.\n";
}

void AdministratorMenu::enrolStudent() {
    listCourses();
    const int courseId = readId("Id of the course");
    listUsers();
    const int studentId = readId("Id of the student");
    admin_.enrolStudent(user_, courseId, studentId);
    out_ << "The student was enrolled.\n";
}

void AdministratorMenu::removeStudent() {
    listCourses();
    const int courseId = readId("Id of the course");
    const int studentId = readId("Id of the student");
    admin_.removeStudent(user_, courseId, studentId);
    out_ << "The student was removed from the course.\n";
}

int AdministratorMenu::readId(const std::string& prompt) {
    return input_.readInt(prompt, 1, 1000000);
}

// Returns false when the Administrator chooses to log out.
bool AdministratorMenu::handle(int choice) {
    switch (choice) {
        case 0:
            return false;
        case 1:
            listUsers();
            break;
        case 2:
            createUser();
            break;
        case 3:
            editUser();
            break;
        case 4:
            deactivateUser();
            break;
        case 5:
            resetPassword();
            break;
        case 6:
            listDepartments();
            break;
        case 7:
            createDepartment();
            break;
        case 8:
            editDepartment();
            break;
        case 9:
            deactivateDepartment();
            break;
        case 10:
            listCourses();
            break;
        case 11:
            createCourse();
            break;
        case 12:
            editCourse();
            break;
        case 13:
            deactivateCourse();
            break;
        case 14:
            assignLecturer();
            break;
        case 15:
            enrolStudent();
            break;
        case 16:
            removeStudent();
            break;
        default:
            break;
    }
    return true;
}

}  // namespace lms
