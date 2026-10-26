// A very small test framework (no outside library, so the tests build offline).
#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace test {

struct Case {
    std::string name;
    std::function<void()> body;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

inline int& checks() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

inline void report(bool ok, const char* expression, const char* file, int line) {
    ++checks();
    if (!ok) {
        ++failures();
        std::cerr << "  FAILED: " << expression << "  (" << file << ":" << line << ")\n";
    }
}

}  // namespace test

#define TEST(name)                                              \
    static void name();                                         \
    static const test::Registrar registrar_##name(#name, name); \
    static void name()

#define CHECK(expression) test::report(static_cast<bool>(expression), #expression, __FILE__, __LINE__)

#define CHECK_THROWS(expression, ExceptionType)                                           \
    do {                                                                                  \
        bool thrown_ = false;                                                             \
        try {                                                                             \
            (void)(expression);                                                           \
        } catch (const ExceptionType&) {                                                  \
            thrown_ = true;                                                               \
        } catch (...) {                                                                   \
        }                                                                                 \
        test::report(thrown_, #expression " throws " #ExceptionType, __FILE__, __LINE__); \
    } while (false)
