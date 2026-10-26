#include <iostream>

#include "TestSupport.h"

int main() {
    int failedCases = 0;
    for (const test::Case& c : test::registry()) {
        const int before = test::failures();
        try {
            c.body();
        } catch (const std::exception& e) {
            ++test::failures();
            std::cerr << "  UNEXPECTED EXCEPTION: " << e.what() << '\n';
        }
        const bool ok = test::failures() == before;
        if (!ok) ++failedCases;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << c.name << '\n';
    }
    std::cout << '\n'
              << test::registry().size() << " tests, " << test::checks() << " checks, " << test::failures()
              << " failures.\n";
    return test::failures() == 0 ? 0 : 1;
}
