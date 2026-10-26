# Scripted sessions through the real program

* `run_demo_session.sh` plays `demo_session.txt` (log in as student, lecturer and Head of Department; submit on time and
  late; mark; approve and change a mark; release; look at the audit log) and checks 14 lines of the output.
* `fuzz_menus.py` sends 60 sessions of wrong or random input (letters for numbers, huge numbers, quotes, bad dates,
  missing files, very long text) and fails if the program stops with an error (NFR-08).

Both need a built program (`build/lms`) and are run from the root of the repository.
