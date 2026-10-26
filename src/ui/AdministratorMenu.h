// Layer 1 (presentation): the menu of the Administrator (UC-02 to UC-05).
#pragma once

#include "AdminService.h"
#include "Menu.h"

namespace lms {

class AdministratorMenu : public Menu {
public:
    AdministratorMenu(InputReader& input, std::ostream& out, User& user, AdminService& admin);

protected:
    void printChoices() override;
    bool handle(int choice) override;

private:
    void listUsers();
    void createUser();
    void editUser();
    void deactivateUser();
    void resetPassword();
    void listDepartments();
    void listCourses();
    void createDepartment();
    void editDepartment();
    void deactivateDepartment();
    void createCourse();
    void editCourse();
    void deactivateCourse();
    void assignLecturer();
    void enrolStudent();
    void removeStudent();
    int readId(const std::string& prompt);  // reads an id typed by the user

    User& user_;
    AdminService& admin_;
};

}  // namespace lms
