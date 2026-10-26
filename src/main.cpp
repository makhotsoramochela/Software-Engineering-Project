// Moderated Learning Management System, version 1.0.
#include <iostream>
#include <string>

#include "Application.h"

int main(int argc, char** argv) {
    lms::ApplicationOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--demo") {
            options.demoData = true;
        } else if (argument == "--db" && i + 1 < argc) {
            options.databasePath = argv[++i];
        } else {
            std::cerr << "Usage: lms [--db <file>] [--demo]\n"
                      << "  --db <file>  the database file (default: lms.db)\n"
                      << "  --demo       fill an empty database with demo data\n";
            return argument == "--help" || argument == "-h" ? 0 : 2;
        }
    }
    lms::Application application(options, std::cin, std::cout);
    return application.run();
}
