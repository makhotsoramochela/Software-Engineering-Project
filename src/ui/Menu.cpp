#include "Menu.h"

#include <exception>

namespace lms {

void Menu::run() {
    bool again = true;
    while (again) {
        try {
            out_ << '\n';
            printChoices();
            const int choice = input_.readChoice("Choose", 0, maxChoice_);
            again = handle(choice);
        } catch (const EndOfInput&) {
            throw;  // the input has ended: leave the whole program
        } catch (const LmsError& error) {
            showError(error);
        } catch (const std::exception& error) {
            // Any other error (for example no memory) must not stop the program either (NFR-08).
            out_ << "Unexpected error: " << error.what() << "\nNothing was changed. Please try again.\n";
        }
    }
}

void Menu::showError(const LmsError& error) {
    out_ << "Error: " << error.what() << '\n';
}

}  // namespace lms
