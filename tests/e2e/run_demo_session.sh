#!/bin/sh
# Plays one complete session of all four roles through the real program (demo data) and checks the main results.
# Run from the root of the repository, after building:   sh tests/e2e/run_demo_session.sh
set -e
BIN="${1:-build/lms}"
DB="$(mktemp -u /tmp/lms_e2e_XXXXXX.db)"
OUT="$(mktemp /tmp/lms_e2e_out_XXXXXX.txt)"
"$BIN" --db "$DB" --demo < tests/e2e/demo_session.txt > "$OUT"
fail=0
check() { if grep -q -F "$1" "$OUT"; then echo "  ok    $1"; else echo "  FAIL  $1"; fail=1; fi; }
echo "Checking the output of the session:"
check "Error: Wrong username or password."                       # FR-01
check "Status: Submitted"                                        # FR-16 receipt (on time)
check "WARNING: this work is 2 day(s) late."                     # FR-18 warning before saving
check "A penalty of 4.0 marks (out of 20)"                       # BR-01: 2 days x 10% of 20
check "Status: Late (2 day(s) late)"                             # FR-18
check "Error: Late work cannot be replaced."                     # BR-03
check "No marks have been released to you yet."                  # BR-02, FR-19
check "Raw mark: 15.0   Days late: 2   Penalty: 4.0   Final mark: 11.0 out of 20"   # FR-13
check "The mark was approved. The status is now Moderated."      # FR-21
check "The final mark was changed and written in the audit log." # FR-14, FR-21
check "Old final mark: 11.0   New final mark: 10.0   Reason: Rubric check: reduces to 10"   # FR-26
check "Old final mark: -   New final mark: 40.0   Reason: initial marking"                   # BR-05
check "Your mark for Assignment 1: Requirements (CS4433) has been released."                 # FR-23
check "Final mark: 40.0 out of 50"                               # FR-19
rm -f "$DB" "$OUT"
[ "$fail" = 0 ] && echo "ALL CHECKS PASSED" || { echo "SOME CHECKS FAILED"; exit 1; }
