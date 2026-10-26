#include "StudentMenu.h"

#include <cstdio>

#include "Format.h"

namespace lms {

namespace {
// The name of the file without its folder, so that only the file name is stored.
std::string baseName(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}
}  // namespace

StudentMenu::StudentMenu(InputReader& input, std::ostream& out, User& user, SubmissionService& submissions)
    : Menu(input, out, 3), user_(user), submissions_(submissions) {}

void StudentMenu::printChoices() {
    out_ << "=== Student: " << user_.getFullName() << " ===\n"
         << "1. My courses and assignments\n"
         << "2. Submit (or resubmit) an assignment\n"
         << "3. My released marks and feedback\n"
         << "0. Log out\n";
}

void StudentMenu::listAssignments() {
    const std::vector<StudentAssignmentRow> rows = submissions_.listMyAssignments(user_);
    if (rows.empty()) {
        out_ << "You have no assignments yet.\n";
        return;
    }
    out_ << format::cell("Id", 5) << format::cell("Course", 10) << format::cell("Assignment", 26)
         << format::cell("Your deadline", 18) << format::cell("Max", 5) << "Status\n";
    for (const StudentAssignmentRow& r : rows) {
        out_ << format::cell(std::to_string(r.assignmentId), 5) << format::cell(r.courseCode, 10)
             << format::cell(r.title, 26) << format::cell(r.deadline.toDisplay(), 18)
             << format::cell(std::to_string(r.maxMark), 5) << r.statusText << '\n';
    }
}

void StudentMenu::submit() {
    listAssignments();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    const std::string path = input_.readLine("Path of the file (at most 5 MB)");
    const std::vector<std::uint8_t> bytes = input_.readFileBytes(path);
    const SubmissionPreview preview = submissions_.prepareSubmission(user_, assignmentId, baseName(path), bytes);

    out_ << "Assignment: " << preview.assignmentTitle << "\nFile: " << preview.fileName << " (" << bytes.size()
         << " bytes)\nYour deadline: " << preview.deadline.toDisplay() << "\nNow: " << preview.checkedAt.toDisplay()
         << '\n';
    if (preview.isResubmission) {
        out_ << "This will replace your earlier file. The earlier file is kept as version " << preview.nextVersionNo - 1
             << ".\n";
    } else if (preview.isLate) {  // FR-18: the student sees the penalty before the file is saved
        out_ << "WARNING: this work is " << preview.daysLate << " day(s) late.\n";
        if (preview.getsZero) {
            out_ << "Work that is more than 3 days late receives the mark 0.\n";
        } else {
            out_ << "A penalty of " << format::number(preview.maxPenaltyMarks) << " marks (out of " << preview.maxMark
                 << ") will be taken off your mark.\n";
        }
        out_ << "Late work cannot be replaced after it is saved.\n";
    } else {
        out_ << "The work is on time. You may replace it until your deadline.\n";
    }
    if (!input_.confirm("Save this file as your submission?")) {
        out_ << "Nothing was saved.\n";
        return;
    }
    const Receipt receipt = submissions_.confirmSubmission(user_, preview);
    out_ << "\nRECEIPT\nAssignment: " << receipt.assignmentTitle << "\nFile: " << receipt.fileName
         << "\nVersion: " << receipt.versionNo << "\nSaved at: " << receipt.submittedAt.toDisplay()
         << "\nStatus: " << displayName(receipt.status);
    if (receipt.daysLate > 0) out_ << " (" << receipt.daysLate << " day(s) late)";
    out_ << '\n';
}

void StudentMenu::showResults() {
    const std::vector<ResultRow> rows = submissions_.listMyResults(user_);
    if (rows.empty()) {
        out_ << "No marks have been released to you yet.\n";
        return;
    }
    for (const ResultRow& r : rows) {
        out_ << r.courseCode << ", " << r.assignmentTitle << '\n'
             << "   Final mark: " << format::number(r.finalMark) << " out of " << r.maxMark;
        if (r.daysLate > 0) {
            out_ << "   (" << r.daysLate << " day(s) late, penalty " << format::number(r.penalty) << ")";
        }
        out_ << "\n   Feedback: " << r.feedback << '\n';
    }
}

bool StudentMenu::handle(int choice) {
    switch (choice) {
        case 0:
            return false;
        case 1:
            listAssignments();
            break;
        case 2:
            submit();
            break;
        case 3:
            showResults();
            break;
        default:
            break;
    }
    return true;
}

}  // namespace lms
