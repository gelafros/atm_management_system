#include "header.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helper: populate a struct Record from a prepared SQLite statement result row.
// Must be called inside a sqlite3_step() == SQLITE_ROW block.
// Expected column order: id, user_id, name, account_nbr, deposit_date,
//                        country, phone, amount, type
// ─────────────────────────────────────────────────────────────────────────────
static void readRecordFromStmt(sqlite3_stmt *stmt, char name[100], struct Record *r)
{
    r->id     = sqlite3_column_int(stmt, 0);
    r->userId = sqlite3_column_int(stmt, 1);

    // Copy the owner name from the result row into the caller's buffer
    strncpy(name, (const char *)sqlite3_column_text(stmt, 2), 99);
    name[99] = '\0';

    r->accountNbr = sqlite3_column_int(stmt, 3);

    // deposit_date is stored as "MM/DD/YYYY" text — parse it back into struct Date
    const char *dateStr = (const char *)sqlite3_column_text(stmt, 4);
    sscanf(dateStr, "%d/%d/%d", &r->deposit.month, &r->deposit.day, &r->deposit.year);

    strncpy(r->country, (const char *)sqlite3_column_text(stmt, 5), 99);
    r->country[99] = '\0';

    // phone is stored as INTEGER — use int64 to handle large international numbers
    r->phone  = (long long)sqlite3_column_int64(stmt, 6);
    r->amount = sqlite3_column_double(stmt, 7);

    strncpy(r->accountType, (const char *)sqlite3_column_text(stmt, 8), 9);
    r->accountType[9] = '\0';
}

// stayOrReturn prompts the user to either retry the current operation, return to the main menu, or exit the program.
// It handles cases where an account is not found (notGood == 0) or simply serves as a navigation prompt.
void stayOrReturn(int notGood, void f(struct User u, sqlite3 *db), struct User u, sqlite3 *db)
{
    int option;
    if (notGood == 0 || notGood == 3)
    {
        system("clear");
        if (notGood == 0)
            printf("%s\n✖ Oops! We couldn't find a record matching that account.%s\n", COLOR_RED, COLOR_RESET);
        else
            printf("%s\n✖ Transfer failed! The recipient user does not exist.%s\n", COLOR_RED, COLOR_RESET);
    invalid:
        printf("\nEnter 0 to try again, 1 to return to main menu and 2 to exit:");
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n');
            option = -1;
        }
        if (option == 0)
        {
            f(u, db);
            return;
        }
        else if (option == 1)
        {
            return;
        }
        else if (option == 2)
        {
            exit(0);
            return;
        }
        else
        {
            printf("%sInvalid operation! Please choose a valid option.%s\n", COLOR_RED, COLOR_RESET);
            goto invalid;
        }
    }
    else
    {
        printf("\nEnter 1 to go to the main menu and 0 to exit:");
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n');
            option = -1;
        }
    }
    if (option == 1)
    {
        system("clear");
    }
    else
    {
        system("clear");
        exit(0);
    }
}

// success displays a positive confirmation message and prompts the user to navigate back to the menu or exit.
// It prevents the terminal from immediately closing before the user can read the result.
void success(struct User u)
{
    (void)u;
    int option;
    printf("%s\n✔ Success! The operation was completed successfully.%s\n\n", COLOR_GREEN, COLOR_RESET);
invalid:
    printf("Enter 1 to go to the main menu and 0 to exit!\n");
    if (scanf("%d", &option) != 1) {
        while (getchar() != '\n');
        option = -1;
    }
    system("clear");
    if (option == 1)
    {
        return;
    }
    else if (option == 0)
    {
        exit(0);
    }
    else
    {
        printf("%sInvalid operation! Please choose a valid option.%s\n", COLOR_RED, COLOR_RESET);
        goto invalid;
    }
}

// CountryCode maps a country name to its international dial code.
struct CountryCode {
    char country[50];
    int code;
};

// countryCodes is a lookup table of country names and their international dial codes.
struct CountryCode countryCodes[] = {
    {"Afghanistan", 93},  {"Albania", 355},    {"Algeria", 213},
    {"Argentina", 54},   {"Australia", 61},   {"Austria", 43},
    {"Belgium", 32},     {"Brazil", 55},      {"Canada", 1},
    {"China", 86},       {"Croatia", 385},    {"Cyprus", 357},
    {"Czech", 420},      {"Denmark", 45},     {"Egypt", 20},
    {"Finland", 358},    {"France", 33},      {"Germany", 49},
    {"Greece", 30},      {"Hungary", 36},     {"India", 91},
    {"Indonesia", 62},   {"Iran", 98},        {"Iraq", 964},
    {"Ireland", 353},    {"Israel", 972},     {"Italy", 39},
    {"Japan", 81},       {"Jordan", 962},     {"Kenya", 254},
    {"Kuwait", 965},     {"Lebanon", 961},    {"Malaysia", 60},
    {"Mexico", 52},      {"Morocco", 212},    {"Netherlands", 31},
    {"Nigeria", 234},    {"Norway", 47},      {"Pakistan", 92},
    {"Philippines", 63}, {"Poland", 48},      {"Portugal", 351},
    {"Romania", 40},     {"Russia", 7},       {"SaudiArabia", 966},
    {"Serbia", 381},     {"Singapore", 65},   {"Slovakia", 421},
    {"Slovenia", 386},   {"SouthAfrica", 27}, {"SouthKorea", 82},
    {"Spain", 34},       {"Sweden", 46},      {"Switzerland", 41},
    {"Syria", 963},      {"Thailand", 66},    {"Tunisia", 216},
    {"Turkey", 90},      {"UAE", 971},        {"UK", 44},
    {"Ukraine", 380},    {"USA", 1},          {"Vietnam", 84},
    {"Yemen", 967},
};

// getCountryCode looks up the international dial code for a given country name.
// Returns the dial code as an int, or -1 if the country is not found.
int getCountryCode(char *country)
{
    int size = sizeof(countryCodes) / sizeof(countryCodes[0]);
    for (int i = 0; i < size; i++)
    {
        if (strcasecmp(countryCodes[i].country, country) == 0)
            return countryCodes[i].code;
    }
    return -1;
}

// createNewAcc prompts the user to input details for a new bank account and saves it to the database.
// It prevents duplicate account numbers for the same user and validates all input fields.
void createNewAcc(struct User u, sqlite3 *db)
{
    struct Record r;
    sqlite3_stmt *stmt;

noAccount:
    system("clear");
    printf("\t\t\t===== New record =====\n");

    // --- Deposit date ---
    // The user enters any valid date (audit uses specific past dates like 10/10/2012
    // to verify interest calculations — we accept any date, not just today)
    printf("\nEnter the deposit date(mm/dd/yyyy):");
    if (scanf("%d/%d/%d", &r.deposit.month, &r.deposit.day, &r.deposit.year) != 3)
    {
        while (getchar() != '\n');
        printf("%s✖ Invalid date format! Please use mm/dd/yyyy format.%s\n", COLOR_RED, COLOR_RESET);
        sleep(3);
        goto noAccount;
    }

enterAccountNbr:
    // --- Account number ---
    printf("\nEnter the account number:");
    if (scanf("%d", &r.accountNbr) != 1)
    {
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
        sleep(3);
        goto enterAccountNbr;
    }

    // --- Check for duplicate account number for this user ---
    // Query: does a record already exist with this user + account number combination?
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name,       -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, r.accountNbr);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // A row was found — this account number is already taken by this user
        sqlite3_finalize(stmt);
        printf("%s✖ This account number is already in use by this user. Please try a different one.%s\n\n", COLOR_RED, COLOR_RESET);
        goto enterAccountNbr;
    }
    sqlite3_finalize(stmt);

    // --- Country ---
    int code = -1;
    while (code == -1)
    {
        printf("\nEnter the country: ");
        scanf("%s", r.country);
        code = getCountryCode(r.country);
        if (code == -1)
            printf("%s✖ Country '%s' not recognized. Please try again.%s\n", COLOR_RED, r.country, COLOR_RESET);
    }
    printf("\n%sCountry code: +%d%s", COLOR_GREEN, code, COLOR_RESET);

    // --- Phone number (must be digits only, 8-15 chars, start with country code) ---
    char phoneStr[20];
    int phoneValid = 0;
    while (!phoneValid)
    {
        printf("\nEnter phone number (with country code e.g. %d...): ", code);
        scanf("%19s", phoneStr);
        while (getchar() != '\n');

        int len = strlen(phoneStr);
        int digitsOnly = 1;
        for (int i = 0; i < len; i++)
        {
            if (phoneStr[i] < '0' || phoneStr[i] > '9')
            {
                digitsOnly = 0;
                break;
            }
        }

        if (!digitsOnly)
            printf("%s✖ Phone number must contain digits only (no spaces or dashes).%s\n", COLOR_RED, COLOR_RESET);
        else if (len < 8 || len > 15)
            printf("%s✖ Phone number must be between 8 and 15 digits.%s\n", COLOR_RED, COLOR_RESET);
        else
        {
            char codeStr[10];
            sprintf(codeStr, "%d", code);
            if (strncmp(phoneStr, codeStr, strlen(codeStr)) != 0)
                printf("%s✖ Phone number must start with the country code %d.%s\n", COLOR_RED, code, COLOR_RESET);
            else
            {
                r.phone = atoll(phoneStr);
                phoneValid = 1;
            }
        }
    }

    // --- Initial deposit amount ---
    while (1)
    {
        printf("\nEnter amount to deposit: $");
        if (scanf("%lf", &r.amount) == 1 && r.amount > 0.0)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid deposit amount! Must be a positive number.%s\n", COLOR_RED, COLOR_RESET);
    }

    // --- Account type selection ---
    int typeChoice = 0;
    while (typeChoice < 1 || typeChoice > 5)
    {
        printf("\nChoose the type of account:\n");
        printf("\t[1] Saving\n");
        printf("\t[2] Current\n");
        printf("\t[3] Fixed - 1 year  (4%% interest)\n");
        printf("\t[4] Fixed - 2 years (5%% interest)\n");
        printf("\t[5] Fixed - 3 years (8%% interest)\n");
        printf("\n\tEnter your choice: ");
        if (scanf("%d", &typeChoice) != 1)
        {
            while (getchar() != '\n');
            typeChoice = 0;
        }
        switch (typeChoice)
        {
            case 1: strcpy(r.accountType, "savings");  break;
            case 2: strcpy(r.accountType, "current"); break;
            case 3: strcpy(r.accountType, "fixed01"); break;
            case 4: strcpy(r.accountType, "fixed02"); break;
            case 5: strcpy(r.accountType, "fixed03"); break;
            default:
                printf("%s✖ Invalid choice. Please enter a number between 1 and 5.%s\n", COLOR_RED, COLOR_RESET);
                typeChoice = 0;
        }
    }

    // --- Format the date as a TEXT string for database storage ---
    // Stored as "MM/DD/YYYY" to match the format expected by the audit
    char dateStr[20];
    sprintf(dateStr, "%d/%d/%d", r.deposit.month, r.deposit.day, r.deposit.year);

    // --- INSERT the new account into the database ---
    // The id column is AUTOINCREMENT — SQLite assigns it automatically
    sqlite3_prepare_v2(db,
        "INSERT INTO records (user_id, name, account_nbr, deposit_date, country, phone, amount, type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);",
        -1, &stmt, NULL);

    sqlite3_bind_int   (stmt, 1, u.id);              // owner's user id (foreign key)
    sqlite3_bind_text  (stmt, 2, u.name,       -1, SQLITE_STATIC);
    sqlite3_bind_int   (stmt, 3, r.accountNbr);
    sqlite3_bind_text  (stmt, 4, dateStr,      -1, SQLITE_STATIC);
    sqlite3_bind_text  (stmt, 5, r.country,    -1, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 6, r.phone);
    sqlite3_bind_double(stmt, 7, r.amount);
    sqlite3_bind_text  (stmt, 8, r.accountType,-1, SQLITE_STATIC);

    sqlite3_step(stmt);     // execute the INSERT
    sqlite3_finalize(stmt); // free the prepared statement

    success(u);
}

// checkAllAccounts retrieves and displays a formatted list of all bank accounts belonging to the logged-in user.
void checkAllAccounts(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int found = 0;

    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);

    // Query: get every record row that belongs to this user
    sqlite3_prepare_v2(db,
        "SELECT id, user_id, name, account_nbr, deposit_date, country, phone, amount, type "
        "FROM records WHERE name = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    // Iterate through each result row — each row is one account
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        struct Record r;
        char userName[100];
        readRecordFromStmt(stmt, userName, &r); // fill struct from current row

        found = 1;
        printf("_____________________\n");
        printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:+%lld \nAmount deposited: $%.2f \nType Of Account:%s\n",
               r.accountNbr,
               r.deposit.month, r.deposit.day, r.deposit.year,
               r.country,
               r.phone,
               r.amount,
               r.accountType);
    }
    sqlite3_finalize(stmt);

    if (!found)
    {
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, checkAllAccounts, u, db);
        return;
    }
    success(u);
}

// checkAcc first lists all accounts belonging to the user, then prompts for an account number.
// It displays full details and calculates the expected interest based on account type.
void checkAcc(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int found = 0;
    double interest;

    system("clear");
    printf("\t\t====== Your accounts =====\n\n");

    // --- First pass: show a summary list of all accounts for this user ---
    sqlite3_prepare_v2(db,
        "SELECT id, user_id, name, account_nbr, deposit_date, country, phone, amount, type "
        "FROM records WHERE name = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        struct Record r;
        char userName[100];
        readRecordFromStmt(stmt, userName, &r);

        found = 1;
        printf("_____________________\n");
        printf("\nAccount number: %d | Type: %s | Balance: $%.2f\n",
               r.accountNbr, r.accountType, r.amount);
    }
    sqlite3_finalize(stmt);

    if (!found)
    {
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, checkAcc, u, db);
        return;
    }

    // --- Second pass: let the user pick a specific account to see in detail ---
    int accNbr;
    printf("\n_____________________\n");
    while (1)
    {
        printf("Enter the account number to see full details and interest: ");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }

    found = 0; // reset for the second query

    // Query: find only the specific account the user chose
    sqlite3_prepare_v2(db,
        "SELECT id, user_id, name, account_nbr, deposit_date, country, phone, amount, type "
        "FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accNbr);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        struct Record r;
        char userName[100];
        readRecordFromStmt(stmt, userName, &r);
        found = 1;

        printf("_____________________\n");
        printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:+%lld \nAmount deposited: $%.2f \nType Of Account:%s\n",
               r.accountNbr,
               r.deposit.month, r.deposit.day, r.deposit.year,
               r.country,
               r.phone,
               r.amount,
               r.accountType);

        // --- Interest calculation ---
        // savings: 7% annual rate paid monthly → divide by 12
        if (strcmp(r.accountType, "savings") == 0)
        {
            interest = (r.amount * 0.07) / 12;
            printf("_____________________\n");
            printf("You will get $%.2f as interest on day %d of every month\n", interest, r.deposit.day);
        }
        // fixed01: 4% rate × 1 year = 4% total, paid at end of year
        else if (strcmp(r.accountType, "fixed01") == 0)
        {
            interest = r.amount * 0.04 * 1;
            printf("_____________________\n");
            printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 1);
        }
        // fixed02: 5% rate × 2 years = 10% total, paid at end of term
        else if (strcmp(r.accountType, "fixed02") == 0)
        {
            interest = r.amount * 0.05 * 2;
            printf("_____________________\n");
            printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 2);
        }
        // fixed03: 8% rate × 3 years = 24% total, paid at end of term
        else if (strcmp(r.accountType, "fixed03") == 0)
        {
            interest = r.amount * 0.08 * 3;
            printf("_____________________\n");
            printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 3);
        }
        // current: no interest applies
        else if (strcmp(r.accountType, "current") == 0)
        {
            printf("_____________________\n");
            printf("You will not get interests because the account is of type current\n");
        }
    }
    sqlite3_finalize(stmt);

    if (!found)
    {
        stayOrReturn(0, checkAcc, u, db);
        return;
    }
    success(u);
}

// userHasAccounts checks if the logged-in user has any accounts in the database.
// Returns 1 if at least one account is found, 0 otherwise.
int userHasAccounts(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;

    // LIMIT 1 keeps this fast — we only need to know if any row exists, not how many
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE name = ? LIMIT 1;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    // SQLITE_ROW = at least one account was found for this user
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

// makeTransaction handles depositing and withdrawing funds for a given account.
// Fixed-term accounts (fixed01, fixed02, fixed03) are blocked from transactions.
// Withdrawals are blocked if the amount exceeds the current balance.
void makeTransaction(struct User u, sqlite3 *db)
{
    if (!userHasAccounts(u, db))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, makeTransaction, u, db);
        return;
    }

    // Ask which account number to transact on
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number:");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }

    // --- Load the account from the database ---
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT id, user_id, name, account_nbr, deposit_date, country, phone, amount, type "
        "FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accNbr);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        // No matching account found for this user
        sqlite3_finalize(stmt);
        stayOrReturn(0, makeTransaction, u, db);
        return;
    }

    struct Record r;
    char userName[100];
    readRecordFromStmt(stmt, userName, &r); // load account data into struct
    sqlite3_finalize(stmt);

    // --- Block transactions on fixed-term accounts ---
    if (strcmp(r.accountType, "fixed01") == 0 ||
        strcmp(r.accountType, "fixed02") == 0 ||
        strcmp(r.accountType, "fixed03") == 0)
    {
        printf("_____________________\n");
        printf("%s\n✖ Transactions (deposits/withdrawals) are not allowed for fixed-term accounts.%s\n", COLOR_RED, COLOR_RESET);
        success(u);
        return;
    }

    // --- Deposit or Withdrawal prompt ---
    int DorW;
    do
    {
        printf("_____________________\n");
        printf("\nDeposit: 1 or Withdrawal: 2 ?");
        if (scanf("%d", &DorW) != 1) {
            while (getchar() != '\n');
            DorW = -1;
        }

        if (DorW == 1)
        {
            // --- Deposit: add money to the account ---
            double deposit;
            printf("_____________________\n");
            while (1)
            {
                printf("\nEnter amount to deposit: $");
                if (scanf("%lf", &deposit) == 1 && deposit > 0.0)
                    break;
                while (getchar() != '\n');
                printf("%s✖ Invalid amount! Must be a positive number.%s\n", COLOR_RED, COLOR_RESET);
            }
            r.amount += deposit; // update the local amount
            printf("_____________________\n");
            printf("New balance: $%.2f\n", r.amount);
        }
        else if (DorW == 2)
        {
            // --- Withdrawal: remove money from the account ---
            double withdraw;
            printf("_____________________\n");
            while (1)
            {
                printf("\nEnter amount to withdraw: $");
                if (scanf("%lf", &withdraw) == 1 && withdraw > 0.0)
                    break;
                while (getchar() != '\n');
                printf("%s✖ Invalid amount! Must be a positive number.%s\n", COLOR_RED, COLOR_RESET);
            }

            if (r.amount >= withdraw)
            {
                r.amount -= withdraw; // update the local amount
                printf("_____________________\n");
                printf("New balance: $%.2f\n", r.amount);
            }
            else
            {
                // Overdraft protection — cannot withdraw more than the balance
                printf("_____________________\n");
                printf("%s\n✖ Withdrawal denied. Insufficient funds.%s\n", COLOR_RED, COLOR_RESET);
                printf("Available balance: $%.2f\n", r.amount);
                success(u);
                return;
            }
        }
    }
    while (DorW != 1 && DorW != 2);

    // --- Save the updated balance back to the database ---
    // Only the amount column changes — all other fields remain the same
    sqlite3_prepare_v2(db,
        "UPDATE records SET amount = ? WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, r.amount);            // new balance
    sqlite3_bind_text  (stmt, 2, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int   (stmt, 3, accNbr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    success(u);
}

// updateAcc allows the user to modify the phone number or country of a specific account.
// Uses a targeted UPDATE query — only the chosen field is changed in the database.
void updateAcc(struct User u, sqlite3 *db)
{
    if (!userHasAccounts(u, db))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, updateAcc, u, db);
        return;
    }

    // Ask which account number to update
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number:");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }

    // --- Verify the account exists for this user ---
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accNbr);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        // Account not found for this user
        sqlite3_finalize(stmt);
        stayOrReturn(0, updateAcc, u, db);
        return;
    }
    sqlite3_finalize(stmt);

    // --- Ask which field to update ---
    int choice;
    do
    {
        printf("_____________________\n");
        printf("What do you want to update?\n");
        printf("Phone number:1  or country:2 ?");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            choice = -1;
        }

        if (choice == 1)
        {
            // --- Update phone number only ---
            long long newPhone;
            printf("_____________________\n");
            while (1)
            {
                printf("\nEnter the new phone number:");
                if (scanf("%lld", &newPhone) == 1)
                    break;
                while (getchar() != '\n');
                printf("%s✖ Invalid phone number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
            }

            // SET only the phone column — other fields unchanged
            sqlite3_prepare_v2(db,
                "UPDATE records SET phone = ? WHERE name = ? AND account_nbr = ?;",
                -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, newPhone);
            sqlite3_bind_text (stmt, 2, u.name, -1, SQLITE_STATIC);
            sqlite3_bind_int  (stmt, 3, accNbr);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        else if (choice == 2)
        {
            // --- Update country only ---
            char newCountry[100];
            printf("_____________________\n");
            printf("\nEnter the new country:");
            scanf("%s", newCountry);

            // SET only the country column — other fields unchanged
            sqlite3_prepare_v2(db,
                "UPDATE records SET country = ? WHERE name = ? AND account_nbr = ?;",
                -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, newCountry, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, u.name,     -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 3, accNbr);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    while (choice != 1 && choice != 2);

    success(u);
}

// removeAcc deletes a specific account from the database using a single DELETE query.
// sqlite3_changes() tells us if the account actually existed and was deleted.
void removeAcc(struct User u, sqlite3 *db)
{
    if (!userHasAccounts(u, db))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, removeAcc, u, db);
        return;
    }

    // Ask which account to delete
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number:");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }

    // DELETE the row matching this user + account number
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "DELETE FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accNbr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // sqlite3_changes() returns the number of rows deleted.
    // If 0, no matching account was found — show an error.
    if (sqlite3_changes(db) == 0)
    {
        stayOrReturn(0, removeAcc, u, db);
        return;
    }

    success(u);
}

// transferOwner reassigns the ownership of a given account to another registered user.
// It verifies both the account and the recipient exist before updating the database.
void transferOwner(struct User u, sqlite3 *db)
{
    if (!userHasAccounts(u, db))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, transferOwner, u, db);
        return;
    }

    // Ask which account to transfer
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number:");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }

    // --- Verify the account exists and belongs to this user ---
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 2, accNbr);

    if (sqlite3_step(stmt) != SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        stayOrReturn(0, transferOwner, u, db);
        return;
    }
    sqlite3_finalize(stmt);

    // --- Ask for the recipient's username ---
    char newUserName[100];
    printf("_____________________\n");
    printf("\nEnter the new owner name:");
    scanf("%s", newUserName);

    // --- Verify the recipient exists in the users table ---
    int newUserId = -1;
    sqlite3_prepare_v2(db,
        "SELECT id FROM users WHERE name = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, newUserName, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Recipient found — read their id so we can store it in the record
        newUserId = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (newUserId == -1)
    {
        // Recipient username not found in the system
        stayOrReturn(3, transferOwner, u, db);
        return;
    }

    // --- Update the record: change both the name and user_id to the new owner ---
    sqlite3_prepare_v2(db,
        "UPDATE records SET name = ?, user_id = ? WHERE name = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, newUserName, -1, SQLITE_STATIC); // new owner's name
    sqlite3_bind_int (stmt, 2, newUserId);                       // new owner's id
    sqlite3_bind_text(stmt, 3, u.name,      -1, SQLITE_STATIC); // current owner's name
    sqlite3_bind_int (stmt, 4, accNbr);                          // account being transferred
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // --- Write a notification file for the new owner ---
    // The file lives at data/notifications/<newowner>.txt
    // mainMenu() checks for this file every time the user enters the menu
    // and displays it once before deleting it.
    char notifPath[300];
    sprintf(notifPath, "./data/notifications/%s.txt", newUserName);

    FILE *nf = fopen(notifPath, "w");
    if (nf)
    {
        fprintf(nf, "Account #%d has been transferred to you by %s.\n", accNbr, u.name);
        fclose(nf);
    }

    success(u);
}
