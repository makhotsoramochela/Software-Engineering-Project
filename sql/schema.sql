-- Moderated LMS: database schema for SQLite (version 1.0)
-- Run "PRAGMA foreign_keys = ON;" on every new connection.
-- Dates and times are text in the form YYYY-MM-DDTHH:MM (local time, to the minute).

PRAGMA foreign_keys = ON;

CREATE TABLE department (
    department_id  INTEGER PRIMARY KEY,
    name           TEXT    NOT NULL UNIQUE CHECK (length(trim(name)) > 0),
    active         INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1))
);

CREATE TABLE user_account (
    user_id        INTEGER PRIMARY KEY,
    username       TEXT    NOT NULL UNIQUE COLLATE NOCASE CHECK (length(trim(username)) > 0),
    full_name      TEXT    NOT NULL CHECK (length(trim(full_name)) > 0),
    password_hash  TEXT    NOT NULL CHECK (length(password_hash) > 0),
    role           TEXT    NOT NULL CHECK (role IN ('ADMINISTRATOR', 'LECTURER', 'HEAD_OF_DEPARTMENT', 'STUDENT')),
    department_id  INTEGER REFERENCES department (department_id),
    active         INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1)),
    failed_logins  INTEGER NOT NULL DEFAULT 0 CHECK (failed_logins BETWEEN 0 AND 4),
    locked_until   TEXT    CHECK (locked_until IS NULL OR strftime('%Y-%m-%dT%H:%M', locked_until) IS locked_until),
    CHECK ((role = 'HEAD_OF_DEPARTMENT') = (department_id IS NOT NULL))
);

-- A department can have only one active Head of Department.
CREATE UNIQUE INDEX one_active_hod_per_department
    ON user_account (department_id)
    WHERE role = 'HEAD_OF_DEPARTMENT' AND active = 1;

CREATE TABLE course (
    course_id      INTEGER PRIMARY KEY,
    code           TEXT    NOT NULL UNIQUE CHECK (length(trim(code)) > 0),
    title          TEXT    NOT NULL CHECK (length(trim(title)) > 0),
    department_id  INTEGER NOT NULL REFERENCES department (department_id),
    active         INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1))
);

CREATE TABLE course_lecturer (
    course_id      INTEGER NOT NULL REFERENCES course (course_id),
    lecturer_id    INTEGER NOT NULL REFERENCES user_account (user_id),
    PRIMARY KEY (course_id, lecturer_id)
);

CREATE TABLE enrolment (
    course_id      INTEGER NOT NULL REFERENCES course (course_id),
    student_id     INTEGER NOT NULL REFERENCES user_account (user_id),
    PRIMARY KEY (course_id, student_id)
);

CREATE TRIGGER course_lecturer_role_check
BEFORE INSERT ON course_lecturer
WHEN (SELECT role FROM user_account WHERE user_id = NEW.lecturer_id) <> 'LECTURER'
BEGIN
    SELECT RAISE(ABORT, 'Only a lecturer can be assigned to a course');
END;

CREATE TRIGGER enrolment_role_check
BEFORE INSERT ON enrolment
WHEN (SELECT role FROM user_account WHERE user_id = NEW.student_id) <> 'STUDENT'
BEGIN
    SELECT RAISE(ABORT, 'Only a student can be enrolled in a course');
END;

CREATE TABLE assignment (
    assignment_id  INTEGER PRIMARY KEY,
    course_id      INTEGER NOT NULL REFERENCES course (course_id),
    title          TEXT    NOT NULL CHECK (length(trim(title)) > 0),
    description    TEXT    NOT NULL CHECK (length(trim(description)) > 0),
    deadline       TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', deadline) IS deadline),
    max_mark       INTEGER NOT NULL CHECK (max_mark BETWEEN 1 AND 100)
);

CREATE TABLE extension (
    extension_id   INTEGER PRIMARY KEY,
    assignment_id  INTEGER NOT NULL REFERENCES assignment (assignment_id),
    student_id     INTEGER NOT NULL REFERENCES user_account (user_id),
    new_deadline   TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', new_deadline) IS new_deadline),
    reason         TEXT    NOT NULL CHECK (length(trim(reason)) > 0),
    granted_by     INTEGER NOT NULL REFERENCES user_account (user_id),
    granted_at     TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', granted_at) IS granted_at)
);

CREATE TABLE submission (
    submission_id  INTEGER PRIMARY KEY,
    assignment_id  INTEGER NOT NULL REFERENCES assignment (assignment_id),
    student_id     INTEGER NOT NULL REFERENCES user_account (user_id),
    status         TEXT    NOT NULL CHECK (status IN ('SUBMITTED', 'LATE', 'MARKED', 'RETURNED', 'MODERATED', 'RELEASED')),
    days_late      INTEGER NOT NULL DEFAULT 0 CHECK (days_late >= 0),
    raw_mark       REAL    CHECK (raw_mark BETWEEN 0 AND 100),
    penalty        REAL    CHECK (penalty >= 0),
    final_mark     REAL    CHECK (final_mark BETWEEN 0 AND 100),
    feedback       TEXT,
    marked_by      INTEGER REFERENCES user_account (user_id),
    UNIQUE (assignment_id, student_id),
    CHECK (
        (status IN ('SUBMITTED', 'LATE')
            AND raw_mark IS NULL AND penalty IS NULL AND final_mark IS NULL
            AND feedback IS NULL AND marked_by IS NULL)
        OR
        (status NOT IN ('SUBMITTED', 'LATE')
            AND raw_mark IS NOT NULL AND penalty IS NOT NULL AND final_mark IS NOT NULL
            AND feedback IS NOT NULL AND length(trim(feedback)) > 0 AND marked_by IS NOT NULL)
    )
);

CREATE TABLE submission_version (
    version_id     INTEGER PRIMARY KEY,
    submission_id  INTEGER NOT NULL REFERENCES submission (submission_id),
    version_no     INTEGER NOT NULL CHECK (version_no >= 1),
    file_name      TEXT    NOT NULL CHECK (length(trim(file_name)) > 0),
    file_content   BLOB    NOT NULL CHECK (length(file_content) BETWEEN 1 AND 5242880),
    submitted_at   TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', submitted_at) IS submitted_at),
    UNIQUE (submission_id, version_no)
);

CREATE TABLE status_change (
    change_id      INTEGER PRIMARY KEY,
    submission_id  INTEGER NOT NULL REFERENCES submission (submission_id),
    from_status    TEXT    CHECK (from_status IS NULL OR from_status IN ('SUBMITTED', 'LATE', 'MARKED', 'RETURNED', 'MODERATED', 'RELEASED')),
    to_status      TEXT    NOT NULL CHECK (to_status IN ('SUBMITTED', 'LATE', 'MARKED', 'RETURNED', 'MODERATED', 'RELEASED')),
    changed_by     INTEGER NOT NULL REFERENCES user_account (user_id),
    changed_at     TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', changed_at) IS changed_at),
    comment        TEXT
);

CREATE TABLE audit_entry (
    audit_id        INTEGER PRIMARY KEY,
    submission_id   INTEGER NOT NULL REFERENCES submission (submission_id),
    changed_by      INTEGER NOT NULL REFERENCES user_account (user_id),
    changed_at      TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', changed_at) IS changed_at),
    old_final_mark  REAL    CHECK (old_final_mark BETWEEN 0 AND 100),
    new_final_mark  REAL    NOT NULL CHECK (new_final_mark BETWEEN 0 AND 100),
    reason          TEXT    NOT NULL CHECK (length(trim(reason)) > 0)
);

-- Audit log entries can never be changed or deleted (NFR-09).
CREATE TRIGGER audit_entry_no_update
BEFORE UPDATE ON audit_entry
BEGIN
    SELECT RAISE(ABORT, 'Audit log entries cannot be changed');
END;

CREATE TRIGGER audit_entry_no_delete
BEFORE DELETE ON audit_entry
BEGIN
    SELECT RAISE(ABORT, 'Audit log entries cannot be deleted');
END;

CREATE TABLE message (
    message_id     INTEGER PRIMARY KEY,
    user_id        INTEGER NOT NULL REFERENCES user_account (user_id),
    body           TEXT    NOT NULL CHECK (length(trim(body)) > 0),
    created_at     TEXT    NOT NULL CHECK (strftime('%Y-%m-%dT%H:%M', created_at) IS created_at),
    seen           INTEGER NOT NULL DEFAULT 0 CHECK (seen IN (0, 1))
);

CREATE INDEX idx_course_department       ON course (department_id);
CREATE INDEX idx_course_lecturer_lect    ON course_lecturer (lecturer_id);
CREATE INDEX idx_enrolment_student       ON enrolment (student_id);
CREATE INDEX idx_assignment_course       ON assignment (course_id);
CREATE INDEX idx_extension_lookup        ON extension (assignment_id, student_id);
CREATE INDEX idx_submission_assign_stat  ON submission (assignment_id, status);
CREATE INDEX idx_submission_student      ON submission (student_id);
CREATE INDEX idx_status_change_submission ON status_change (submission_id);
CREATE INDEX idx_audit_submission        ON audit_entry (submission_id);
CREATE INDEX idx_message_user_seen       ON message (user_id, seen);
