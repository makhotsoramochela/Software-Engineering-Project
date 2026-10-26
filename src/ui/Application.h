// Layer 1 (presentation): where the program starts. Application is the only class that creates the
// objects of all layers and joins them (rule AR-8, the "composition root"). Each object receives what it
// needs through its constructor.
#pragma once

#include <iostream>
#include <string>

namespace lms {

struct ApplicationOptions {
    std::string databasePath = "lms.db";
    bool demoData = false;  // fill an empty database with demo users, courses and assignments
};

class Application {
public:
    Application(ApplicationOptions options, std::istream& in, std::ostream& out);
    // Returns the exit code of the program: 0 for a normal end, 1 if the program cannot start.
    int run();

private:
    ApplicationOptions options_;
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace lms
