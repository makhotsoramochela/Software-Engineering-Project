The Presentation video link is here:
##Presentation Video: 
Presentation: https://drive.google.com/file/d/1KbvFrabdTuGk0oNb6BrROtiy6B4ATIPo/view?usp=drive_link



Moderated LMS, version 1.0

A console Learning Management System with moderated marking, written in C++17 with SQLite.
It works offline, on one computer, with one data file.

1. What you need

 Tool, Version and Why 

 -C++17 compiler (g++ or clang++) (g++ 9 or later, clang 10 or later).It builds the program 
 -CMake (3.16 or later). It also builds the program 
 -SQLite 3 with headers (3.31 or later).For the database 
 -libsodium with headers (1.0.18 or later). For password hasThing (Argon2id)
 -pkg-config (any). Lets CMake find libsodium 

Ubuntu or Debian (tested on Ubuntu 24.04 with g++ 13.3, CMake 3.28, SQLite 3.45, libsodium 1.0.18):

sudo apt install build-essential cmake pkg-config libsqlite3-dev libsodium-dev


Other systems: install the same four libraries and tools with the package manager of the system
(for example `brew install cmake pkg-config sqlite libsodium` on macOS). The code has some `#if` parts for
Windows, but it was not tested on Windows.

2. Build


cd lms
cmake -S . -B build
cmake --build build


This makes two programs: build/lms (the system) and build/tests/lms_tests (the automated tests).
CMake builds in the Release mode by default. To switch the tests off: cmake -S . -B build -DLMS_BUILD_TESTS=OFF.

3. Run the tests


ctest --test-dir build --output-on-failure


or run `build/tests/lms_tests` directly. The expected end is `36 tests, 302 checks, 0 failures.`


Line coverage of the business layer (NFR-10 asks for at least 70%; the measured value is 85.8%):

cmake -S . -B build-cov -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage -O0"
cmake --build build-cov && build-cov/tests/lms_tests
cd build-cov && gcov -n CMakeFiles/lms_business.dir/src/business/*.gcda     # "Lines executed" for each file


Two more checks are not part of `ctest`, because they depend on the computer and on typed input:

* `build/tests/lms_bench` fills a database to the size of NFR-13 (1,000 users, 100 courses, 20,000 submissions)
  and prints the timings of NFR-01 and NFR-02.
* `tests/e2e/run_demo_session.sh` plays a complete session of all four roles through the real program, and
  `python3 tests/e2e/fuzz_menus.py` sends it wrong input (NFR-08). See `tests/e2e/README.md`.

4. Start the program

build/lms                    # uses the file lms.db in the current folder
build/lms --db /path/to/file.db
build/lms --demo             # A database is filled with demonstration users, courses and assignments


**First start (without `--demo`):** the database file is created and the program asks you to create the first
Administrator account (username, full name, password). There is no default password.
The password needs at least 8 characters, with at least one letter and one digit (BR-08).

**Demonstration data** (`--db demo`, only for an empty database). For showing the system.

| Username                                               | Password  | Role            
|---                                                     |---        |---              
| admin                                                  | Admin1234 | Administrator 
| lecturer1 (teaches CS4433, CS3302), lecturer2 (MA2101) | Demo1234  | Lecturer 
| hod_cs, hod_maths                                      | Demo1234  | Head of Department 
| student1 to student6                                   | Demo1234  | Student 

5. Using the system

After the log in, the menu of the role is shown. Type the number of a choice and press Enter. Everything is
text and works with the keyboard only. Dates are typed as `2026-11-30 17:00`.

 Role and what the menu offers 

 Administrator: users (create, edit, deactivate, new password), departments, courses, assign lecturers, enrol and remove students 
 Lecturer: own courses, create assignments, give an extension to one student, see submissions, mark a submission (raw mark, then the penalty and final mark are shown, then feedback), save a submitted file
 Head of Department: see marked work of the department, approve, change a final mark (with a reason), send back (with a comment), release one or all of an assignment, summary, audit log 
 Student: courses and assignments with the own deadline and status, submit or resubmit a file (late work is accepted after a warning that shows the penalty), released marks and feedback 

A lecturer or student sees a message about released or returned work at the next log in.

 6. Where the data is, and backup

All data is in the one SQLite file (`lms.db` by default), including the submitted files. To make a backup, stop the
program and copy the file. The schema is in `sql/schema.sql`; the program builds it into the executable and creates
it in a new file. The file has the version number 1 (`PRAGMA user_version`); a file with another number is refused.

 7. Password hashing and its setting

Passwords are never stored as text. `SodiumPasswordHasher` (libsodium) stores only an **Argon2id** hash with its own
random salt, as one text. Argon2id was chosen because it is the winner of the Password Hashing Competition and the
current recommendation of OWASP for new systems; libsodium is a well-known, audited library with a simple interface
that has no weak option to choose by mistake.

NFR-03 asks for one check to take between 100 and 500 milliseconds on the reference computer. The default
setting is 4 passes and 64 MiB of memory (`src/data/SodiumPasswordHasher.cpp`). Calibration on the reference
computer: run `build/tests/lms_tests` (it prints the time of one hash). If the time is under 100 ms, raise the
number of passes; if it is over 500 ms, lower it; then build again. Old hashes stay valid because the setting is
stored inside each hash.

 8. Structure of the source code


src/business/   Layer 2: domain model, 8 services, penalty rule, interfaces (Ports.h), exceptions
src/data/       Layer 3: SQLite repositories, Database, SystemClock, SodiumPasswordHasher
src/ui/         Layer 1: menus, InputReader, Application (the only place that joins all objects)
sql/schema.sql  the SQLite schema (tables, keys, constraints, triggers)
tests/          automated tests (and tests/e2e: scripted sessions)


The layers are enforced by CMake: `lms_business` has no include path to SQLite or libsodium, and
`lms_ui` is linked only to `lms_business`, so a menu cannot call SQL (rule AR-1).




