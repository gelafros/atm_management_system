# ATM Management System

## Description
A command-line ATM management system built in C with SQLite. Users can register, log in, manage bank accounts, make transactions, and transfer account ownership securely.

## Features
- **User Authentication** — Login and Registration with unique username enforcement
- **Password Encryption** — Passwords are encrypted before storage using Caesar cipher
- **Create Accounts** — Savings, Current, Fixed01 (1yr), Fixed02 (2yr), Fixed03 (3yr)
- **Check Account Details** — View individual account info with interest calculations
- **Check Owned Accounts** — List all accounts belonging to the logged-in user
- **Update Accounts** — Modify phone number or country for an existing account
- **Make Transactions** — Deposit and withdraw with overdraft protection (blocked for fixed-term accounts)
- **Remove Accounts** — Delete any owned account
- **Transfer Ownership** — Transfer an account to another registered user
- **Instant Notifications** — Real-time transfer alerts via named pipes (FIFO) and child processes
- **SQLite Database** — Relational database with foreign keys, replacing flat-file storage
- **Color-coded TUI** — Enhanced terminal interface with ANSI colors and box-drawing

## Prerequisites
- GCC or any C compiler
- SQLite3 development library (`libsqlite3-dev`)

## How to Compile and Run
```bash
cd atm-system
make
./atm
```

To clean build artifacts:
```bash
make clean
```

## File Structure
```
atm-system/
├── Makefile
├── data/
│   ├── users.txt        # Legacy seed data (imported on first run)
│   ├── records.txt      # Legacy seed data (imported on first run)
│   └── atm.db           # SQLite database (auto-created)
└── src/
    ├── header.h         # Shared structs, prototypes, color macros
    ├── main.c           # Entry point, menus, FIFO notification listener
    ├── auth.c           # Login, password masking, password retrieval
    ├── system.c         # All ATM operations (CRUD, transactions, transfers)
    └── db.c             # SQLite initialization and legacy data migration
```
## Authors

- [gelafros](https://github.com/gelafros)

## Security note

This is an educational project. Passwords are obfuscated with a Caesar cipher (shift +3), which is **not** secure. A real system must store salted hashes (bcrypt or argon2) and never store passwords reversibly.

## License

MIT, see [LICENSE](LICENSE).
