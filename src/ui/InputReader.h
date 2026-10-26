// Layer 1 (presentation): reads what the user types and checks its form (design: Figure 3.13).
// Bad input never stops the program: a question is asked again up to MAX_TRIES times, then a
// ValidationError is thrown and the menu loop shows it (AR-6, NFR-08).
#pragma once

#include <cstdint>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "DateTime.h"

namespace lms {

// Thrown when the input stream ends (for example Ctrl+D). The program then leaves cleanly.
class EndOfInput : public std::runtime_error {
public:
    EndOfInput() : std::runtime_error("The input has ended.") {}
};

class InputReader {
public:
    InputReader(std::istream& in, std::ostream& out);

    std::string readLine(const std::string& prompt);
    // Like readLine, but the typed characters are not shown when the input is a terminal.
    std::string readPassword(const std::string& prompt);
    int readChoice(const std::string& prompt, int min, int max);
    int readInt(const std::string& prompt, int min, int max);
    double readDecimal(const std::string& prompt, double min, double max);
    DateTime readDateTime(const std::string& prompt);
    bool confirm(const std::string& prompt);

    // Files. Both throw ValidationError with a clear message if the file cannot be used.
    std::vector<std::uint8_t> readFileBytes(const std::string& path);
    void writeFileBytes(const std::string& path, const std::vector<std::uint8_t>& bytes);

    static constexpr int MAX_TRIES = 3;

private:
    std::istream& in_;
    std::ostream& out_;
};

}  // namespace lms
