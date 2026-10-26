#include "HeadOfDepartmentMenu.h"

#include "Format.h"

namespace lms {

HeadOfDepartmentMenu::HeadOfDepartmentMenu(InputReader& input, std::ostream& out, User& user,
                                           ModerationService& moderation, AuditService& audit, MarkingService& marking)
    : Menu(input, out, 9), user_(user), moderation_(moderation), audit_(audit), marking_(marking) {}

void HeadOfDepartmentMenu::printChoices() {
    out_ << "=== Head of Department: " << user_.getFullName() << " ===\n"
         << "1. See the marked submissions of my department\n"
         << "2. Approve a mark\n"
         << "3. Change a final mark (with a reason)\n"
         << "4. Send a submission back to the lecturer (with a comment)\n"
         << "5. Release one moderated submission\n"
         << "6. Release all moderated submissions of an assignment\n"
         << "7. Summary of each assignment\n"
         << "8. See the audit log\n"
         << "9. Save a submitted file to this computer\n"
         << "0. Log out\n";
}

void HeadOfDepartmentMenu::reviewMarked() {
    const std::vector<SubmissionRow> rows = moderation_.listMarked(user_);
    if (rows.empty()) {
        out_ << "There are no marked submissions waiting for moderation.\n";
        return;
    }
    for (const SubmissionRow& r : rows) {
        out_ << "Submission " << r.submissionId << ": " << r.courseCode << ", " << r.assignmentTitle << ", student "
             << r.studentName << '\n'
             << "   Lecturer: " << r.markedByName << "   Raw mark: " << format::number(r.rawMark)
             << "   Days late: " << r.daysLate << "   Penalty: " << format::number(r.penalty)
             << "   Final mark: " << format::number(r.finalMark) << " out of " << r.maxMark << '\n'
             << "   Feedback: " << r.feedback.value_or("") << '\n';
    }
}

void HeadOfDepartmentMenu::printSummary() {
    out_ << format::cell("Id", 5) << format::cell("Course", 10) << format::cell("Assignment", 30)
         << format::cell("Marked", 8) << format::cell("Moderated", 11) << "Released\n";
    for (const AssignmentSummary& s : moderation_.summary(user_)) {
        out_ << format::cell(std::to_string(s.assignmentId), 5) << format::cell(s.courseCode, 10)
             << format::cell(s.title, 30) << format::cell(std::to_string(s.marked), 8)
             << format::cell(std::to_string(s.moderated), 11) << s.released << '\n';
    }
}

void HeadOfDepartmentMenu::releaseOne() {
    printSummary();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    const std::vector<SubmissionRow> rows = moderation_.listModerated(user_, assignmentId);
    if (rows.empty()) {
        out_ << "No moderated submissions for this assignment.\n";
        return;
    }
    out_ << format::cell("Id", 5) << format::cell("Student", 24) << "Final mark\n";
    for (const SubmissionRow& r : rows) {
        out_ << format::cell(std::to_string(r.submissionId), 5) << format::cell(r.studentName, 24)
             << format::number(r.finalMark) << '\n';
    }
    const int submissionId = input_.readInt("Submission id", 1, 1000000);
    if (!input_.confirm("Release this mark to the student?")) return;
    moderation_.release(user_, submissionId);
    out_ << "The mark was released.\n";
}

void HeadOfDepartmentMenu::releaseAll() {
    printSummary();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    if (!input_.confirm("Release all moderated submissions of this assignment?")) return;
    const int count = moderation_.releaseAssignment(user_, assignmentId);
    out_ << count << " submission(s) were released.\n";
}

void HeadOfDepartmentMenu::showAuditLog() {
    const std::vector<AuditEntry> entries = audit_.listForMyDepartment(user_);
    if (entries.empty()) {
        out_ << "The audit log has no entries for your department.\n";
        return;
    }
    for (const AuditEntry& e : entries) {
        out_ << e.getChangedAt().toDisplay() << "  " << e.getChangedByName() << "  " << e.getCourseCode() << ", "
             << e.getAssignmentTitle() << ", student " << e.getStudentName() << '\n'
             << "   Old final mark: " << format::number(e.getOldFinalMark())
             << "   New final mark: " << format::number(e.getNewFinalMark()) << "   Reason: " << e.getReason() << '\n';
    }
}

void HeadOfDepartmentMenu::saveFile() {
    reviewMarked();
    const int submissionId = input_.readInt("Submission id", 1, 1000000);
    const StoredFile file = marking_.exportFile(user_, submissionId);
    std::string path = input_.readLine("Save as (empty = " + file.fileName + " in the current folder)");
    if (path.empty()) path = file.fileName;
    input_.writeFileBytes(path, file.content);
    out_ << "The file was saved as " << path << ".\n";
}

void HeadOfDepartmentMenu::approveMark() {
    reviewMarked();
    const int id = input_.readInt("Id of the submission to approve", 1, 1000000);
    if (!input_.confirm("Approve the mark of submission " + std::to_string(id) + "?")) return;
    moderation_.approve(user_, id);
    out_ << "The mark was approved. The status is now Moderated.\n";
}

void HeadOfDepartmentMenu::changeFinalMark() {
    reviewMarked();
    const int id = input_.readInt("Id of the submission", 1, 1000000);
    const double newFinal = input_.readDecimal("New final mark", 0, 100);
    const std::string reason = input_.readLine("Reason (required)");
    if (!input_.confirm("Save the new final mark?")) return;
    moderation_.adjust(user_, id, newFinal, reason);
    out_ << "The final mark was changed and written in the audit log. The status is now Moderated.\n";
}

void HeadOfDepartmentMenu::sendBack() {
    reviewMarked();
    const int id = input_.readInt("Id of the submission", 1, 1000000);
    const std::string comment = input_.readLine("Comment for the lecturer (required)");
    if (!input_.confirm("Send the submission back to the lecturer?")) return;
    moderation_.returnToLecturer(user_, id, comment);
    out_ << "The submission was sent back. The status is now Returned.\n";
}

// Returns false when the Head of Department chooses to log out.
bool HeadOfDepartmentMenu::handle(int choice) {
    switch (choice) {
        case 0:
            return false;
        case 1:
            reviewMarked();
            break;
        case 2:
            approveMark();
            break;
        case 3:
            changeFinalMark();
            break;
        case 4:
            sendBack();
            break;
        case 5:
            releaseOne();
            break;
        case 6:
            releaseAll();
            break;
        case 7:
            printSummary();
            break;
        case 8:
            showAuditLog();
            break;
        case 9:
            saveFile();
            break;
        default:
            break;
    }
    return true;
}

}  // namespace lms
