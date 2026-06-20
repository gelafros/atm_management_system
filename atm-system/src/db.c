#include "header.h"

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

    // Create tables if they do not already exist
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "   id       INTEGER PRIMARY KEY,"
        "   name     TEXT UNIQUE NOT NULL,"
        "   password TEXT NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS records ("
        "   id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "   user_id      INTEGER NOT NULL,"
        "   name         TEXT NOT NULL,"
        "   account_nbr  INTEGER NOT NULL,"
        "   deposit_date TEXT NOT NULL,"
        "   country      TEXT NOT NULL,"
        "   phone        INTEGER NOT NULL,"
        "   amount       REAL NOT NULL,"
        "   type         TEXT NOT NULL"
        ");";

    char *err = NULL;
    if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK)
    {
        printf("Error creating tables: %s\n", err);
        sqlite3_free(err);
        sqlite3_close(db);
        exit(1);
    }

    return db;
}
