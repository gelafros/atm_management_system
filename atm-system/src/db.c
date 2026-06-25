#include "header.h"

// seedDB imports existing data from the legacy .txt files into the SQLite database.
// It only runs if the tables are empty — so it is safe to call on every startup
// without risking duplicate entries on subsequent runs.
static void seedDB(sqlite3 *db)
{
    sqlite3_stmt *stmt;
    char *err = NULL;

    // ── Seed users ────────────────────────────────────────────────────────────
    // Check if the users table already has rows
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM users;", -1, &stmt, NULL);
    sqlite3_step(stmt);
    int userCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (userCount == 0)
    {
        // Table is empty — read users.txt and insert each user
        FILE *fp = fopen("./data/users.txt", "r");
        if (fp != NULL)
        {
            int id;
            char name[50], password[50];

            // Begin a transaction so all inserts commit atomically (faster + safer)
            sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

            while (fscanf(fp, "%d %s %s", &id, name, password) == 3)
            {
                // Encrypt the password using a Caesar cipher (shift by +3) before saving
                // because users.txt contains plaintext passwords but the application expects encrypted ones.
                int p_idx = 0;
                while (password[p_idx] != '\0')
                {
                    password[p_idx] = password[p_idx] + 3;
                    p_idx++;
                }

                // INSERT each user — use the original id from the file
                sqlite3_prepare_v2(db,
                    "INSERT OR IGNORE INTO users (id, name, password) VALUES (?, ?, ?);",
                    -1, &stmt, NULL);
                sqlite3_bind_int (stmt, 1, id);
                sqlite3_bind_text(stmt, 2, name,     -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, password, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }

            sqlite3_exec(db, "COMMIT;", NULL, NULL, &err);
            fclose(fp);
            printf("%s✔ Seeded users from users.txt into database.%s\n", COLOR_GREEN, COLOR_RESET);
        }
    }

    // ── Seed records ──────────────────────────────────────────────────────────
    // Check if the records table already has rows
    sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM records;", -1, &stmt, NULL);
    sqlite3_step(stmt);
    int recCount = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    if (recCount == 0)
    {
        // Table is empty — read records.txt and insert each record
        FILE *fp = fopen("./data/records.txt", "r");
        if (fp != NULL)
        {
            int id, userId, accountNbr, month, day, year;
            char name[100], country[100], accountType[10];
            long long phone;
            double amount;

            sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

            // Parse the same format used by the old getAccountFromFile:
            // id user_id name account_nbr MM/DD/YYYY country phone amount type
            while (fscanf(fp, "%d %d %s %d %d/%d/%d %s %lld %lf %s",
                          &id, &userId, name, &accountNbr,
                          &month, &day, &year,
                          country, &phone, &amount, accountType) == 11)
            {
                // Format the date back into a "MM/DD/YYYY" text string for the DB
                char dateStr[20];
                sprintf(dateStr, "%d/%d/%d", month, day, year);

                // Prepare INSERT statement - notice we do not include the redundant 'name' column anymore.
                sqlite3_prepare_v2(db,
                    "INSERT OR IGNORE INTO records "
                    "(id, user_id, account_nbr, deposit_date, country, phone, amount, type) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
                    -1, &stmt, NULL);
                sqlite3_bind_int   (stmt, 1, id);
                sqlite3_bind_int   (stmt, 2, userId);
                sqlite3_bind_int   (stmt, 3, accountNbr);
                sqlite3_bind_text  (stmt, 4, dateStr,     -1, SQLITE_STATIC);
                sqlite3_bind_text  (stmt, 5, country,     -1, SQLITE_STATIC);
                sqlite3_bind_int64 (stmt, 6, phone);
                sqlite3_bind_double(stmt, 7, amount);
                sqlite3_bind_text  (stmt, 8, accountType, -1, SQLITE_STATIC);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }

            sqlite3_exec(db, "COMMIT;", NULL, NULL, &err);
            fclose(fp);
            printf("%s✔ Seeded records from records.txt into database.%s\n", COLOR_GREEN, COLOR_RESET);
        }
    }
}

// initDB opens (or creates) the SQLite database file and ensures
// both the users and records tables exist before returning the db pointer.
sqlite3 *initDB()
{
    sqlite3 *db;

    // Open or create the database file
    if (sqlite3_open("./data/atm.db", &db) != SQLITE_OK)
    {
        printf("Error opening database: %s\n", sqlite3_errmsg(db));
        exit(1);
    }

    // Create tables if they do not already exist.
    // The 'records' table is now normalized: 'name' is removed (since it's a duplicate of users.name),
    // and a FOREIGN KEY links it directly to the users table.
    // The 'notifications' table is deleted as we use named pipes (FIFOs) for instant notifications instead.
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "   id       INTEGER PRIMARY KEY,"
        "   name     TEXT UNIQUE NOT NULL,"
        "   password TEXT NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS records ("
        "   id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   user_id      INTEGER NOT NULL,"
        "   account_nbr  INTEGER NOT NULL,"
        "   deposit_date TEXT NOT NULL,"
        "   country      TEXT NOT NULL,"
        "   phone        INTEGER NOT NULL,"
        "   amount       REAL NOT NULL,"
        "   type         TEXT NOT NULL,"
        "   FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
        ");";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK)
    {
        printf("Error creating tables: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        exit(1);
    }

    // Import legacy .txt data into the DB on first run (skipped if tables already have data)
    seedDB(db);

    return db;
}

