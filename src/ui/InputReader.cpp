#include "InputReader.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>

#include "Exceptions.h"
#include "SubmissionService.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace lms {

namespace {

std::string trim(const std::string& s) {
    std::size_t first = 0;
    std::size_t last = s.size();
    while (first < last && (s[first] == ' ' || s[first] == '\t' || s[first] == '\r')) ++first;
    while (last > first && (s[last - 1] == ' ' || s[last - 1] == '\t' || s[last - 1] == '\r')) --last;
    return s.substr(first, last - first);
}

// Turns the echo of the terminal off for the life of the object (for passwords).
class EchoOff {
public:
    EchoOff() {
#if defined(_WIN32)
        handle_ = GetStdHandle(STD_INPUT_HANDLE);
        if (GetConsoleMode(handle_, &mode_)) {
            changed_ = SetConsoleMode(handle_, mode_ & ~static_cast<DWORD>(ENABLE_ECHO_INPUT)) != 0;
        }
#elif defined(__unix__) || defined(__APPLE__)
        if (isatty(STDIN_FILENO) != 0 && tcgetattr(STDIN_FILENO, &saved_) == 0) {
            termios changed = saved_;
            changed.c_lflag &= static_cast<tcflag_t>(~ECHO);
            changed_ = tcsetattr(STDIN_FILENO, TCSANOW, &changed) == 0;
        }
#endif
    }
    ~EchoOff() {
#if defined(_WIN32)
        if (changed_) SetConsoleMode(handle_, mode_);
#elif defined(__unix__) || defined(__APPLE__)
        if (changed_) tcsetattr(STDIN_FILENO, TCSANOW, &saved_);
#endif
    }
    bool active() const { return changed_; }
    EchoOff(const EchoOff&) = delete;
    EchoOff& operator=(const EchoOff&) = delete;

private:
    bool changed_ = false;
#if defined(_WIN32)
    HANDLE handle_ = nullptr;
    DWORD mode_ = 0;
#elif defined(__unix__) || defined(__APPLE__)
    termios saved_{};
#endif
};

}  // namespace

InputReader::InputReader(std::istream& in, std::ostream& out) : in_(in), out_(out) {}

std::string InputReader::readLine(const std::string& prompt) {
    out_ << prompt << ": " << std::flush;
    std::string line;
    if (!std::getline(in_, line)) throw EndOfInput();
    return trim(line);
}

std::string InputReader::readPassword(const std::string& prompt) {
    EchoOff echo;
    out_ << prompt << ": " << std::flush;
    std::string line;
    const bool ok = static_cast<bool>(std::getline(in_, line));
    if (echo.active()) out_ << '\n';
    if (!ok) throw EndOfInput();
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return line;  // spaces are part of a password, so it is not trimmed
}

int InputReader::readChoice(const std::string& prompt, int min, int max) {
    for (;;) {  // a menu asks again until the choice is valid
        const std::string text = readLine(prompt);
        char* end = nullptr;
        errno = 0;
        const long value = std::strtol(text.c_str(), &end, 10);
        if (!text.empty() && end != nullptr && *end == '\0' && errno == 0 && value >= min && value <= max) {
            return static_cast<int>(value);
        }
        out_ << "Please type a number from " << min << " to " << max << ".\n";
    }
}

int InputReader::readInt(const std::string& prompt, int min, int max) {
    for (int attempt = 1; attempt <= MAX_TRIES; ++attempt) {
        const std::string text = readLine(prompt);
        char* end = nullptr;
        errno = 0;
        const long value = std::strtol(text.c_str(), &end, 10);
        if (!text.empty() && end != nullptr && *end == '\0' && errno == 0 && value >= min && value <= max) {
            return static_cast<int>(value);
        }
        out_ << "Please type a whole number from " << min << " to " << max << ".\n";
    }
    throw ValidationError("Too many wrong entries. The action was cancelled.");
}

double InputReader::readDecimal(const std::string& prompt, double min, double max) {
    for (int attempt = 1; attempt <= MAX_TRIES; ++attempt) {
        std::string text = readLine(prompt);
        for (char& c : text) {
            if (c == ',') c = '.';
        }
        char* end = nullptr;
        errno = 0;
        const double value = std::strtod(text.c_str(), &end);
        if (!text.empty() && end != nullptr && *end == '\0' && errno == 0 && std::isfinite(value) && value >= min &&
            value <= max) {
            return value;
        }
        out_ << "Please type a number from " << min << " to " << max << ".\n";
    }
    throw ValidationError("Too many wrong entries. The action was cancelled.");
}

DateTime InputReader::readDateTime(const std::string& prompt) {
    for (int attempt = 1; attempt <= MAX_TRIES; ++attempt) {
        const std::string text = readLine(prompt + " (YYYY-MM-DD HH:MM)");
        try {
            return DateTime::parse(text);
        } catch (const ValidationError& e) {
            out_ << e.what() << '\n';
        }
    }
    throw ValidationError("Too many wrong entries. The action was cancelled.");
}

bool InputReader::confirm(const std::string& prompt) {
    for (;;) {
        std::string answer = readLine(prompt + " (y/n)");
        for (char& c : answer) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (answer == "y" || answer == "yes") return true;
        if (answer == "n" || answer == "no") return false;
        out_ << "Please type y or n.\n";
    }
}

std::vector<std::uint8_t> InputReader::readFileBytes(const std::string& path) {
    if (path.empty()) throw ValidationError("The file name must not be empty.");
    std::ifstream file(path, std::ios::binary);
    if (!file) throw ValidationError("The file cannot be opened: " + path);
    // At most one byte more than the limit is read, so a huge file does not fill the memory.
    const std::size_t limit = SubmissionService::MAX_BYTES;
    std::vector<std::uint8_t> bytes(limit + 1);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    bytes.resize(static_cast<std::size_t>(file.gcount()));
    if (file.bad()) throw ValidationError("The file cannot be read: " + path);
    if (bytes.empty()) throw ValidationError("The file is empty: " + path);
    if (bytes.size() > limit) throw ValidationError("The file is larger than 5 MB: " + path);
    return bytes;
}

void InputReader::writeFileBytes(const std::string& path, const std::vector<std::uint8_t>& bytes) {
    if (path.empty()) throw ValidationError("The file name must not be empty.");
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) throw ValidationError("The file cannot be created: " + path);
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file) throw ValidationError("The file could not be written: " + path);
}

}  // namespace lms
