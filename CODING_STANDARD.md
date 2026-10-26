 Coding standard

This is the coding standard of the Moderated LMS project.

 1. Language and build

* C++17, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`). No other language in the program.
* One build system: CMake. A new .cpp file in src/business, src/data or src/ui is found by CMake without change.
* Warnings are on (`-Wall -Wextra -Wpedantic -Wshadow -Wconversion`). A change is not accepted if it makes a warning.

 2. Layout of the code

* Layers follow the design (architecture rules AR-1 to AR-10 of the design document):
  * src/business (Layer 2) never includes sqlite3.h or sodium.h, never prints or reads the keyboard, and
    reads the time only through Clock.
  * src/data (Layer 3) holds all SQL. Every value goes in a `?` placeholder; SQL is never built by joining typed text.
  * src/ui (Layer 1) calls only services and InputReader. It holds no business rule and no SQL.
  * Only Application (`src/ui/Application.cpp`) creates the objects and joins them through constructors.
* One class (or one closely joined group) for each `.h` / `.cpp` pair. The file has the name of the class.
* A header starts with `#pragma once` and a one-line comment that says what the file is and which layer it is in.
  A header includes only what it needs.

 3. Names

 Thing, Style and Example 

 -class, struct, enum, enum value type: Uses PascalCase style. Eg MarkingService
 -function, method, variable: Uses camelCase style. Eg saveMark, rawMark
 -private data member: Uses camelCase with a trailing `_`. Eg submissions_
 -constant, enumerator, macro: Uses UPPER_SNAKE_CASE style.Eg MAX_BYTES, Role::HEAD_OF_DEPARTMENT
 -namespace: Uses lowercase style. Eg lms, lms::validate
 -file: Uses the class name. Eg DateTime.cpp 

A name says what the thing is or does, in full words. No names with one letter except a short loop counter or a
well-known mathematical name in a small scope.

 4. Format

* Four spaces, no tabs. A line has at most 120 characters. The format is set in .clang-format and applied with
  clang-format -i. The code is committed in that format.
* One statement on a line. Braces are used for every `if`, `for` and `while` that has more than one line.
* public first, then protected, then private.

 5. Design rules for classes

* A class has one reason to change. A rule that may change (the late penalty, the hashing library, the clock) is
  behind an interface (`PenaltyStrategy`, `PasswordHasher`, `Clock`).
* A class receives what it needs through its constructor as a reference to an interface. No global variable, no singleton.
* Use inheritance only for "is a" (`User` and its four roles) and for interfaces. All interfaces have a virtual destructor.
* A base class that is used polymorphically cannot be copied by mistake (`Menu` deletes its copy operations).

 6. Errors and safety

* Errors are reported with exceptions of the family `LmsError` (`ValidationError`, `PermissionError`, `NotFoundError`,
  `RuleError`, `DatabaseError`, `AuthenticationError`). No error code, no `exit()` inside a class.
* Only the menu loop (`Menu::run`) and `Application::run` catch exceptions. They show a message and go on.
* Every service method checks first who the user is and what the user may do (`requirePermission`, own course,
  own department) and then checks the input (`validate::`). Wrong input never reaches the database.
* A method that changes data does it in one `Transaction`. The transaction is committed at the end and rolled back by
  its destructor if an exception comes first.
* Resources have an owner: RAII (`Statement`, `Transaction`, `Database`), `std::unique_ptr` for owned objects.
  No `new` or `delete` in application code, no `malloc`, no raw owning pointer, no C-style cast.
* A password is handled only in `AuthService` and `AdminService`, goes straight to `PasswordHasher`, and is never
  stored, logged or printed.
* Prefer `const` for everything that does not change. Pass a large object as `const&`.
* Magic numbers have a name (`MAX_WRONG`, `LOCK_MINUTES`, `RATE_PERCENT`) and a comment with the requirement.

 7. Comments

* Comment the **why**, with the id of the requirement or rule, not what the next line does.
* Every class and every public method of a service has a short comment on what it does and what it throws or
  requires. A reader should be able to follow a requirement from the SRS to the code with `grep FR-18`.

 8. Tools and checks before a commit

1. cmake --build  has no warning.
2. ctest --test-dir build has 0 failures. A new rule or a changed rule has a test that fails without the change.
3. find src -name '*.cpp' | xargs clang-tidy -p build --quiet` prints no finding (checks in `.clang-tidy`:
   `bugprone-*`, `clang-analyzer-*`, `performance-*` and some `cppcoreguidelines-*`, `modernize-*`).
4. clang-format` makes no change.
5. The commit message says what changed and why, with the requirement id when there is one.

 9. Git

* One logical change in one commit. The `main` branch always builds and passes the tests.
* The database file `lms.db`, the `build/` folder and any file with real data are never committed (`.gitignore`).
