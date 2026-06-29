#ifndef HEADER_H
#define HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sqlite3.h>

// Systems programming headers for named pipes (FIFOs), signal handling, and error checking
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_RESET   "\x1b[0m"
#define STYLE_BOLD    "\x1b[1m"
#define STYLE_DIM     "\x1b[2m"

struct Date
{
    int month, day, year;
};

// all fields for each record of an account
struct Record
{
    int id;
    int userId;
    char name[100];
    char country[100];
    long long phone;
    char accountType[10];
    int accountNbr;
    double amount;
    struct Date deposit;
    struct Date withdraw;
};

struct User
{
    int id;
    char name[50];
    char password[50];
};

// authentication functions
void loginMenu(char a[50], char pass[50]);

const char *getPassword(struct User u, sqlite3 *db);
void getMaskedPassword(char *password);

// system function
void createNewAcc(struct User u, sqlite3 *db);
void mainMenu(struct User u, sqlite3 *db);
void checkAllAccounts(struct User u, sqlite3 *db);
void checkAcc(struct User u, sqlite3 *db);
void makeTransaction(struct User u, sqlite3 *db);
void updateAcc(struct User u, sqlite3 *db);
void removeAcc(struct User u, sqlite3 *db);
void transferOwner(struct User u, sqlite3 *db);
void displayUserAccounts(struct User u, sqlite3 *db);

// database functions
sqlite3 *initDB();

#endif