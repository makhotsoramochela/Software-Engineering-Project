// Layer 1 (presentation): the menu of the student (UC-15 to UC-19).
#pragma once

#include "Menu.h"
#include "SubmissionService.h"

namespace lms {

class StudentMenu : public Menu {
public:
    StudentMenu(InputReader& input, std::ostream& out, User& user, SubmissionService& submissions);

protected:
    void printChoices() override;
    bool handle(int choice) override;

private:
    void listAssignments();
    void submit();
    void showResults();

    User& user_;
    SubmissionService& submissions_;
};

}  // namespace lms
