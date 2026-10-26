#include "Application.h"

#include <memory>

#include "AdminService.h"
#include "AdministratorMenu.h"
#include "AssignmentService.h"
#include "AuditService.h"
#include "AuthService.h"
#include "Database.h"
#include "Exceptions.h"
#include "HeadOfDepartmentMenu.h"
#include "InputReader.h"
#include "LecturerMenu.h"
#include "LoginMenu.h"
#include "MarkingService.h"
#include "MessageService.h"
#include "ModerationService.h"
#include "PenaltyStrategy.h"
#include "SodiumPasswordHasher.h"
#include "SqliteRepositories.h"
#include "StudentMenu.h"
#include "SubmissionService.h"
#include "SystemClock.h"

namespace lms {

namespace {

// Demo data, for showing the system (viva, tests by hand). All demo accounts have the password Demo1234.
void seedDemoData(AdminService& admin, AssignmentService& assignments, IUserRepository& users, Clock& clock,
                  const User& adminUser) {
    const std::string password = "Demo1234";
    const int cs = admin.createDepartment(adminUser, "Computer Science");
    const int maths = admin.createDepartment(adminUser, "Mathematics");
    const int se = admin.createCourse(adminUser, "CS4433", "Software Engineering", cs);
    const int db = admin.createCourse(adminUser, "CS3302", "Databases", cs);
    const int calc = admin.createCourse(adminUser, "MA2101", "Calculus", maths);
    const int l1 = admin.createUser(adminUser, "lecturer1", "Thabo Mokoena", password, Role::LECTURER, std::nullopt);
    const int l2 = admin.createUser(adminUser, "lecturer2", "Palesa Nthane", password, Role::LECTURER, std::nullopt);
    admin.createUser(adminUser, "hod_cs", "Dr Lineo Ramokoena", password, Role::HEAD_OF_DEPARTMENT, cs);
    admin.createUser(adminUser, "hod_maths", "Dr Retselisitsoe Molapo", password, Role::HEAD_OF_DEPARTMENT, maths);
    admin.assignLecturer(adminUser, se, l1);
    admin.assignLecturer(adminUser, db, l1);
    admin.assignLecturer(adminUser, calc, l2);
    const char* names[] = {"Lerato Sello", "Tumelo Khati", "Mpho Letsie", "Neo Makara", "Karabo Thamae", "Refiloe Tau"};
    for (int i = 0; i < 6; ++i) {
        const int student = admin.createUser(adminUser, "student" + std::to_string(i + 1), names[i], password,
                                             Role::STUDENT, std::nullopt);
        admin.enrolStudent(adminUser, se, student);
        admin.enrolStudent(adminUser, db, student);
        if (i < 3) admin.enrolStudent(adminUser, calc, student);
    }
    const auto lecturer = users.findByUsername("lecturer1");
    const DateTime now = clock.now();
    assignments.createAssignment(*lecturer, se, "Assignment 1: Requirements", "Write the SRS of a small system.",
                                 now.plusMinutes(7LL * 24 * 60), 50);
    assignments.createAssignment(*lecturer, se, "Assignment 0: Warm-up (deadline passed)",
                                 "A short task. Its deadline has passed, so work is late.",
                                 now.plusMinutes(-2LL * 24 * 60), 20);
}

// The first Administrator is typed in at the very first start (there is no default password).
void askForFirstAdministrator(AdminService& admin, InputReader& input, std::ostream& out) {
    out << "This is the first start. Create the first Administrator account.\n";
    const std::string username = input.readLine("Username");
    const std::string fullName = input.readLine("Full name");
    const std::string password = input.readPassword("Password (at least 8 characters, a letter and a digit)");
    const std::string again = input.readPassword("Type the password again");
    if (password != again) throw ValidationError("The two passwords are not the same.");
    admin.createFirstAdministrator(username, fullName, password);
    out << "The Administrator account was created.\n";
}

}  // namespace

Application::Application(ApplicationOptions options, std::istream& in, std::ostream& out)
    : options_(std::move(options)), in_(in), out_(out) {}

int Application::run() {
    try {
        // AR-8: all objects are created here and joined through their constructors.
        Database database(options_.databasePath);
        SystemClock clock;
        SodiumPasswordHasher hasher;
        TenPercentPerDayPenalty penalty;
        SqliteUserRepository users(database);
        SqliteCourseRepository courses(database);
        SqliteAssignmentRepository assignments(database);
        SqliteSubmissionRepository submissions(database);
        SqliteAuditRepository audits(database);
        SqliteMessageRepository messages(database);

        AuthService auth(users, hasher, clock, database);
        AdminService admin(users, courses, hasher, database);
        MessageService messageService(messages, clock);
        AuditService audit(audits, clock);
        AssignmentService assignmentService(assignments, courses, submissions, users, penalty, clock, database);
        SubmissionService submissionService(submissions, assignments, courses, assignmentService, penalty, clock,
                                            database);
        MarkingService marking(submissions, assignments, courses, penalty, audit, clock, database);
        ModerationService moderation(submissions, assignments, courses, audit, messageService, clock, database);

        InputReader input(in_, out_);

        if (users.list().empty()) {  // first start of the system
            if (options_.demoData) {
                out_ << "Creating demo data. Administrator: admin / Admin1234. All other demo accounts: Demo1234.\n";
                admin.createFirstAdministrator("admin", "System Administrator", "Admin1234");
                const auto adminUser = users.findByUsername("admin");
                seedDemoData(admin, assignmentService, users, clock, *adminUser);
            } else {
                askForFirstAdministrator(admin, input, out_);
            }
        }

        // Log in, open the menu of the role, and come back to the log in when the user logs out.
        LoginMenu loginMenu(input, out_, auth, messageService);
        while (true) {
            loginMenu.run();
            std::unique_ptr<User> user = loginMenu.takeLoggedInUser();
            if (!user) break;  // the person chose Exit
            switch (user->role()) {
                case Role::ADMINISTRATOR:
                    AdministratorMenu(input, out_, *user, admin).run();
                    break;
                case Role::LECTURER:
                    LecturerMenu(input, out_, *user, assignmentService, marking).run();
                    break;
                case Role::HEAD_OF_DEPARTMENT:
                    HeadOfDepartmentMenu(input, out_, *user, moderation, audit, marking).run();
                    break;
                case Role::STUDENT:
                    StudentMenu(input, out_, *user, submissionService).run();
                    break;
            }
        }
        out_ << "Goodbye.\n";
        return 0;
    } catch (const EndOfInput&) {
        out_ << "\nGoodbye.\n";
        return 0;
    } catch (const LmsError& error) {
        out_ << "The program cannot start: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        out_ << "The program cannot start: " << error.what() << '\n';
        return 1;
    }
}

}  // namespace lms
