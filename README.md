# ATM Management System — C, SQLite3 & Linux Named-Pipe IPC

[![C](https://img.shields.io/badge/Language-C11-A8B9CC?logo=c&logoColor=black)](https://en.cppreference.com/w/c)
[![SQLite](https://img.shields.io/badge/SQLite-3-003B57?logo=sqlite&logoColor=white)](https://www.sqlite.org/)
[![CI](https://github.com/gelafros/atm_management_system/actions/workflows/ci.yml/badge.svg)](https://github.com/gelafros/atm_management_system/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

A command-line banking and ATM management system written in **C** backed by **SQLite3** relational persistence and **POSIX Named Pipes (`mkfifo`)** for real-time inter-process notifications across concurrent user terminal sessions.

---

## Key Engineering Highlights

- **Normalized Relational Persistence (`SQLite3`)**:
  Replaces flat-file `.txt` storage with a normalized schema (`users` and `records` linked via `FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE`), automatic legacy data migration inside atomic SQLite transactions, and parameterized prepared statements (`sqlite3_prepare_v2` / `sqlite3_bind_*`) to prevent SQL injection.
- **Salted SHA-256 Password Hashing & Terminal Masking (`termios`)**:
  Captures passwords with canonical mode and terminal echo disabled (`~ICANON & ~ECHO`), and hashes credentials using a self-contained **FIPS 180-4 SHA-256** implementation salted per username (`atm-v1:<username>:<password>`) so plaintext passwords are never stored in the database.
- **Real-Time Inter-Process Communication (`fork` + `mkfifo`)**:
  Upon login, the session spawns a background child listener (`fork()`) blocking on a per-user named pipe (`./data/fifo_<username>`). When another user transfers account ownership in a separate terminal session, the writer pushes an IPC payload over the FIFO, delivering an instant terminal alert to the recipient and cleaning up via `atexit()` / `SIGTERM`.
- **Financial & Interest Engine**:
  Supports `savings` (7% annual interest paid monthly on the deposit day), `fixed01` (4%, 1-year lock), `fixed02` (5%, 2-year lock), `fixed03` (8%, 3-year lock), and `current` (no interest), enforcing fixed-term transaction locks and overdraft protection.

---

## Project Structure

```text
atm-system/
├── Makefile                 # Strict C build (-Wall -Wextra -lsqlite3)
├── data/
│   ├── users.txt            # Legacy user seed data (migrated on first startup)
│   ├── records.txt          # Legacy account seed data (migrated on first startup)
│   └── atm.db               # SQLite database (auto-generated at runtime)
└── src/
    ├── header.h             # Shared structs, ANSI TUI macros & prototypes
    ├── main.c               # Entry point, TUI menus & POSIX FIFO child listener
    ├── auth.c               # Salted SHA-256 hashing, termios masking & login queries
    ├── system.c             # Account CRUD, interest calculation, transactions & transfers
    └── db.c                 # SQLite schema initialization & transactional data seeding
```

---

## Getting Started

### Prerequisites

- GCC or Clang C compiler
- SQLite3 development headers (`sudo apt-get install libsqlite3-dev`)

### Build & Run

```bash
cd atm-system
make
./atm
```

Clean build artifacts:

```bash
make clean
```

---

## Author

- **Georgios Elafros** ([@gelafros](https://github.com/gelafros))

## License

Released under the [MIT License](LICENSE).
