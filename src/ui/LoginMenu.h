// Layer 1 (presentation): the first menu. UC-01 Log in and FR-23 (messages at the next log in).
#pragma once

#include <memory>

#include "AuthService.h"
#include "MessageService.h"
#include "Menu.h"

namespace lms {

class LoginMenu : public Menu {
public:
    LoginMenu(InputReader& input, std::ostream& out, AuthService& auth, MessageService& messages);

    // After run() has ended: the user who logged in, or nothing if the person chose Exit.
    // The caller then opens the menu of the role of this user.
    std::unique_ptr<User> takeLoggedInUser();

protected:
    void printChoices() override;
    bool handle(int choice) override;

private:
    AuthService& auth_;
    MessageService& messages_;
    std::unique_ptr<User> loggedInUser_;
};

}  // namespace lms
