// Layer 1 (presentation): the menu of the lecturer (UC-06 to UC-09; FR-09 to FR-13).
#pragma once

#include "AssignmentService.h"
#include "MarkingService.h"
#include "Menu.h"

namespace lms {

class LecturerMenu : public Menu {
public:
    LecturerMenu(InputReader& input, std::ostream& out, User& user, AssignmentService& assignments,
                 MarkingService& marking);

protected:
    void printChoices() override;
    bool handle(int choice) override;

private:
    void listCourses();
    void listAssignments();
    void printSubmissions(const std::vector<SubmissionRow>& rows);
    void createAssignment();
    void grantExtension();
    void markSubmission();
    void saveFile();

    User& user_;
    AssignmentService& assignments_;
    MarkingService& marking_;
};

}  // namespace lms
