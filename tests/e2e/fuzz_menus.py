"""Sends wrong and random input to the real program (NFR-08): the program must never stop with an error.
Run from the root of the repository, after building:   python3 tests/e2e/fuzz_menus.py [runs] [path-to-lms]
"""
import os
import random
import subprocess
import sys
import tempfile

RUNS = int(sys.argv[1]) if len(sys.argv) > 1 else 60
BIN = sys.argv[2] if len(sys.argv) > 2 else "build/lms"
random.seed(7)  # the same input every time
JUNK = ["", " ", "abc", "-1", "0", "99999999999999999999", "1e309", "nan", "inf", "' OR 1=1 --", "\x01\x02", "é☃",
        "A" * 5000, "2026-13-45 99:99", "2026-11-10 12:00", "y", "n", "yes", "/nonexistent/file", "/etc/passwd", "/tmp",
        "../../x", "40", "4.55", "-5", "Demo1234", "Admin1234"] + [str(n) for n in range(1, 17)]
USERS = [("admin", "Admin1234"), ("lecturer1", "Demo1234"), ("hod_cs", "Demo1234"), ("student1", "Demo1234"),
         ("student2", "Demo1234")]

problems = 0
for run in range(RUNS):
    db = os.path.join(tempfile.gettempdir(), f"lms_fuzz_{os.getpid()}_{run}.db")
    lines = []
    for _ in range(4):
        user, password = random.choice(USERS)
        lines += ["1", user, password]
        lines += [random.choice(JUNK) if random.random() < 0.7 else str(random.randint(0, 16))
                  for _ in range(random.randint(10, 60))]
        lines += ["0"] * 3
    result = subprocess.run([BIN, "--db", db, "--demo"], input=("\n".join(lines) + "\n").encode("utf-8", "surrogateescape"),
                            capture_output=True, timeout=120)
    out = result.stdout.decode("utf-8", "replace")
    if result.returncode != 0 or result.stderr or "Unexpected error" in out:
        problems += 1
        print(f"run {run}: exit {result.returncode} {result.stderr.decode(errors='replace')[:200]}")
    if os.path.exists(db):
        os.remove(db)
print(f"{RUNS} sessions of wrong input, {problems} problems")
sys.exit(1 if problems else 0)
