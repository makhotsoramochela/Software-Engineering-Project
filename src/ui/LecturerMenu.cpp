#include "LecturerMenu.h"

#include "Format.h"

namespace lms {

LecturerMenu::LecturerMenu(InputReader& input, std::ostream& out, User& user, AssignmentService& assignments,
                           MarkingService& marking)
    : Menu(input, out, 7), user_(user), assignments_(assignments), marking_(marking) {}

void LecturerMenu::printChoices() {
    out_ << "=== Lecturer: " << user_.getFullName() << " ===\n"
         << "1. My courses\n"
         << "2. Create an assignment\n"
         << "3. My assignments\n"
         << "4. Give an extension to a student\n"
         << "5. See the submissions of an assignment\n"
         << "6. Mark a submission\n"
         << "7. Save a submitted file to this computer\n"
         << "0. Log out\n";
}

void LecturerMenu::listCourses() {
    out_ << format::cell("Id", 5) << format::cell("Code", 12) << "Title\n";
    for (const Course& c : assignments_.listMyCourses(user_)) {
        out_ << format::cell(std::to_string(c.getId()), 5) << format::cell(c.getCode(), 12) << c.getTitle() << '\n';
    }
}

void LecturerMenu::listAssignments() {
    out_ << format::cell("Id", 5) << format::cell("Course", 10) << format::cell("Title", 28)
         << format::cell("Deadline", 18) << "Max mark\n";
    for (const AssignmentInfo& a : assignments_.listMine(user_)) {
        out_ << format::cell(std::to_string(a.assignmentId), 5) << format::cell(a.courseCode, 10)
             << format::cell(a.title, 28) << format::cell(a.deadline.toDisplay(), 18) << a.maxMark << '\n';
    }
}

void LecturerMenu::printSubmissions(const std::vector<SubmissionRow>& rows) {
    if (rows.empty()) {
        out_ << "There are no submissions for this assignment yet.\n";
        return;
    }
    out_ << format::cell("Id", 5) << format::cell("Student", 24) << format::cell("Status", 11)
         << format::cell("Submitted", 18) << format::cell("Days late", 10) << format::cell("Raw", 6)
         << format::cell("Penalty", 8) << "Final\n";
    for (const SubmissionRow& r : rows) {
        out_ << format::cell(std::to_string(r.submissionId), 5) << format::cell(r.studentName, 24)
             << format::cell(displayName(r.status), 11) << format::cell(r.submittedAt.toDisplay(), 18)
             << format::cell(std::to_string(r.daysLate), 10) << format::cell(format::number(r.rawMark), 6)
             << format::cell(format::number(r.penalty), 8) << format::number(r.finalMark) << '\n';
    }
}

void LecturerMenu::createAssignment() {
    listCourses();
    const int courseId = input_.readInt("Course id", 1, 1000000);
    const std::string title = input_.readLine("Title");
    const std::string description = input_.readLine("Description");
    const DateTime deadline = input_.readDateTime("Deadline");
    const int maxMark = input_.readInt("Maximum mark (1 to 100)", 1, 100);
    const int id = assignments_.createAssignment(user_, courseId, title, description, deadline, maxMark);
    out_ << "The assignment was created. The id is " << id << ".\n";
}

void LecturerMenu::grantExtension() {
    listAssignments();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    out_ << format::cell("Id", 5) << format::cell("Username", 18) << "Name\n";
    for (const auto& s : assignments_.listEnrolledStudents(user_, assignmentId)) {
        out_ << format::cell(std::to_string(s->getId()), 5) << format::cell(s->getUsername(), 18) << s->getFullName()
             << '\n';
    }
    const int studentId = input_.readInt("Student id", 1, 1000000);
    out_ << "The deadline of this student is now " << assignments_.deadlineFor(assignmentId, studentId).toDisplay()
         << ".\n";
    const DateTime newDeadline = input_.readDateTime("New deadline");
    const std::string reason = input_.readLine("Reason");
    assignments_.grantExtension(user_, assignmentId, studentId, newDeadline, reason);
    out_ << "The extension was saved.\n";
}

void LecturerMenu::markSubmission() {
    listAssignments();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    const std::vector<SubmissionRow> rows = marking_.listSubmissions(user_, assignmentId);
    printSubmissions(rows);
    if (rows.empty()) return;
    const int submissionId = input_.readInt("Submission id", 1, 1000000);
    const SubmissionRow* chosen = nullptr;
    for (const SubmissionRow& r : rows) {
        if (r.submissionId == submissionId) chosen = &r;
    }
    if (chosen == nullptr) throw NotFoundError("That submission is not in the list.");
    const double rawMark = input_.readDecimal("Raw mark (0 to " + std::to_string(chosen->maxMark) + ")", 0, 100);
    const MarkPreview preview = marking_.previewMark(user_, submissionId, rawMark);  // FR-13
    out_ << "Raw mark: " << format::number(preview.rawMark) << "   Days late: " << preview.daysLate
         << "   Penalty: " << format::number(preview.penalty) << "   Final mark: " << format::number(preview.finalMark)
         << " out of " << preview.maxMark << '\n';
    const std::string feedback = input_.readLine("Feedback (required)");
    std::string reason;
    if (chosen->status == SubmissionStatus::MARKED || chosen->status == SubmissionStatus::RETURNED) {
        reason = input_.readLine("Reason for the change (required)");  // BR-06
    }
    if (!input_.confirm("Save this mark?")) {
        out_ << "Nothing was saved.\n";
        return;
    }
    marking_.saveMark(user_, submissionId, rawMark, feedback, reason);
    out_ << "The mark was saved. The status is now Marked.\n";
}

void LecturerMenu::saveFile() {
    listAssignments();
    const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
    printSubmissions(marking_.listSubmissions(user_, assignmentId));
    const int submissionId = input_.readInt("Submission id", 1, 1000000);
    const StoredFile file = marking_.exportFile(user_, submissionId);
    std::string path = input_.readLine("Save as (empty = " + file.fileName + " in the current folder)");
    if (path.empty()) path = file.fileName;
    input_.writeFileBytes(path, file.content);
    out_ << "The file was saved as " << path << ".\n";
}

bool LecturerMenu::handle(int choice) {
    switch (choice) {
        case 0:
            return false;
        case 1:
            listCourses();
            break;
        case 2:
            createAssignment();
            break;
        case 3:
            listAssignments();
            break;
        case 4:
            grantExtension();
            break;
        case 5: {
            listAssignments();
            const int assignmentId = input_.readInt("Assignment id", 1, 1000000);
            printSubmissions(marking_.listSubmissions(user_, assignmentId));
            break;
        }
        case 6:
            markSubmission();
            break;
        case 7:
            saveFile();
            break;
        default:
            break;
    }
    return true;
}

}  // namespace lms
