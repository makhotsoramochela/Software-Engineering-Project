// Layer 1 (presentation): the menu of the Head of Department (UC-12 to UC-14, UC-20).
#pragma once

#include "AuditService.h"
#include "MarkingService.h"
#include "Menu.h"
#include "ModerationService.h"

namespace lms {

class HeadOfDepartmentMenu : public Menu {
public:
    HeadOfDepartmentMenu(InputReader& input, std::ostream& out, User& user, ModerationService& moderation,
                         AuditService& audit, MarkingService& marking);

protected:
    void printChoices() override;
    bool handle(int choice) override;

private:
    void reviewMarked();
    void approveMark();
    void changeFinalMark();
    void sendBack();
    void printSummary();
    void releaseOne();
    void releaseAll();
    void showAuditLog();
    void saveFile();

    User& user_;
    ModerationService& moderation_;
    AuditService& audit_;
    MarkingService& marking_;
};

}  // namespace lms
