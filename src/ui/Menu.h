// Layer 1 (presentation): the abstract Menu with the Template Method pattern (design: Table 3.5).
// run() is the loop that all menus share. Subclasses only say what to print and what a choice does.
// AR-1: a menu calls only the services and InputReader. AR-6: this is the one place that catches errors.
#pragma once

#include <ostream>

#include "Exceptions.h"
#include "InputReader.h"

namespace lms {

class Menu {
public:
    Menu(InputReader& input, std::ostream& out, int maxChoice) : input_(input), out_(out), maxChoice_(maxChoice) {}
    virtual ~Menu() = default;
    Menu(const Menu&) = delete;
    Menu& operator=(const Menu&) = delete;

    // The loop: print the choices, read a choice, do it, and show any error without stopping.
    void run();

protected:
    virtual void printChoices() = 0;
    // Returns false when the user chose to leave this menu.
    virtual bool handle(int choice) = 0;
    void showError(const LmsError& error);

    InputReader& input_;
    std::ostream& out_;
    int maxChoice_;
};

}  // namespace lms
