#include "LoginMenu.h"

namespace lms {

LoginMenu::LoginMenu(InputReader& input, std::ostream& out, AuthService& auth, MessageService& messages)
    : Menu(input, out, 1), auth_(auth), messages_(messages) {}

std::unique_ptr<User> LoginMenu::takeLoggedInUser() {
    return std::move(loggedInUser_);
}

void LoginMenu::printChoices() {
    out_ << "=== Moderated LMS ===\n"
         << "1. Log in\n"
         << "0. Exit\n";
}

bool LoginMenu::handle(int choice) {
    if (choice == 0) return false;
    const std::string username = input_.readLine("Username");
    const std::string password = input_.readPassword("Password");
    std::unique_ptr<User> user = auth_.login(username, password);
    out_ << "\nWelcome, " << user->getFullName() << " (" << displayName(user->role()) << ").\n";
    for (const Message& m : messages_.takeWaiting(*user)) {  // FR-23
        out_ << "  Message (" << m.getCreatedAt().toDisplay() << "): " << m.getBody() << '\n';
    }
    loggedInUser_ = std::move(user);
    return false;  // leave this menu; the caller opens the menu of the role
}

}  // namespace lms
