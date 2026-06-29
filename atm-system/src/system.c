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

// validatePhoneNumber validates that the phone number contains digits only
// and is between 8 and 15 digits.
int validatePhoneNumber(const char *phoneStr)
{
    int len = strlen(phoneStr);
    for (int i = 0; i < len; i++)
    {
        if (phoneStr[i] < '0' || phoneStr[i] > '9')
        {
            printf("%s✖ Phone number must contain digits only.%s\n", COLOR_RED, COLOR_RESET);
            return 0;
        }
    }
    if (len < 8 || len > 15)
    {
        printf("%s✖ Phone number must be between 8 and 15 digits.%s\n", COLOR_RED, COLOR_RESET);
        return 0;
    }
    return 1;
}

// createNewAcc prompts the user to input details for a new bank account and saves it to the database.
// It prevents duplicate account numbers for the same user and validates all input fields.
void createNewAcc(struct User u, sqlite3 *db)
{
    struct Record r;
    sqlite3_stmt *stmt;

noAccount:
    printf("\n\t\t" COLOR_CYAN "┌──────────────────────────────────────────────┐" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "│" STYLE_BOLD COLOR_GREEN "             📝 NEW BANK RECORD               " COLOR_CYAN "│" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "└──────────────────────────────────────────────┘" COLOR_RESET "\n");

    // --- Deposit date ---
    // The user enters any valid date 
    // to verify interest calculations — we accept any date, not just today)
    printf("\nEnter the date (mm/dd/yyyy) or enter 0 to return to main menu: ");
    char dateInput[50];
    if (scanf("%49s", dateInput) != 1)
    {
        while (getchar() != '\n');
        goto noAccount;
    }
    if (strcmp(dateInput, "0") == 0)
    {
        return;
    }
    if (sscanf(dateInput, "%d/%d/%d", &r.deposit.month, &r.deposit.day, &r.deposit.year) != 3)
    {
        printf("%s✖ Invalid date format! Please use mm/dd/yyyy format.%s\n", COLOR_RED, COLOR_RESET);
        sleep(3);
        goto noAccount;
    }
    if (r.deposit.month < 1 || r.deposit.month > 12 ||
        r.deposit.day < 1 || r.deposit.day > 31 ||
        r.deposit.year < 1900 || r.deposit.year > 2100)
    {
        printf("%s✖ Invalid date! Month must be 1-12, day 1-31, year 1900-2100.%s\n", COLOR_RED, COLOR_RESET);
        sleep(3);
        goto noAccount;
    }

enterAccountNbr:
    // --- Account number ---
    printf("\nEnter the account number (or press 0 to auto-generate, or enter -1 to cancel): ");
    if (scanf("%d", &r.accountNbr) != 1)
    {
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
        sleep(3);
        goto enterAccountNbr;
    }

    if (r.accountNbr == -1)
    {
        printf("\nAccount creation cancelled.\n");
        sleep(2);
        return;
    }

    if (r.accountNbr == 0)
    {
        // Auto-generate: find max account_nbr in records table globally and add 1
        sqlite3_prepare_v2(db, "SELECT COALESCE(MAX(account_nbr), 0) + 1 FROM records;", -1, &stmt, NULL);
        sqlite3_step(stmt);
        r.accountNbr = sqlite3_column_int(stmt, 0);
        sqlite3_finalize(stmt);
        printf("%s✔ Generated account number: %d%s\n", COLOR_GREEN, r.accountNbr, COLOR_RESET);
    }
    else if (r.accountNbr < 0)
    {
        printf("%s✖ Account number must be positive!%s\n", COLOR_RED, COLOR_RESET);
        sleep(2);
        goto enterAccountNbr;
    }
    else
    {
        // --- Check for duplicate account number for this user ---
        // Query: does a record already exist with this user ID + account number combination?
        sqlite3_prepare_v2(db,
            "SELECT id FROM records WHERE user_id = ? AND account_nbr = ?;",
            -1, &stmt, NULL);
        sqlite3_bind_int (stmt, 1, u.id);
        sqlite3_bind_int (stmt, 2, r.accountNbr);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // A row was found — this account number is already taken by this user
            sqlite3_finalize(stmt);
            printf("%s✖ This account number is already in use by this user. Please try a different one.%s\n\n", COLOR_RED, COLOR_RESET);
            goto enterAccountNbr;
        }
        sqlite3_finalize(stmt);
    }

    // --- Country ---
    int code = -1;
    while (code == -1)
    {
        printf("\nEnter the country (or enter 0 to cancel): ");
        // Use a width limit of 99 to prevent buffer overflow on the 100-byte country buffer.
        scanf("%99s", r.country);
        if (strcmp(r.country, "0") == 0)
        {
            printf("\nAccount creation cancelled.\n");
            sleep(2);
            return;
        }
        code = getCountryCode(r.country);
        if (code == -1)
            printf("%s✖ Country '%s' not recognized. Please try again.%s\n", COLOR_RED, r.country, COLOR_RESET);
    }
    printf("\n%sCountry code: +%d%s", COLOR_GREEN, code, COLOR_RESET);

    // --- Phone number (must be digits only, 8-15 chars, start with any valid country code) ---
    char phoneStr[20];
    int phoneValid = 0;
    while (!phoneValid)
    {
        printf("\nEnter phone number (with country code e.g. %d...) or enter 0 to cancel: ", code);
        scanf("%19s", phoneStr);
        while (getchar() != '\n');

        if (strcmp(phoneStr, "0") == 0)
        {
            printf("\nAccount creation cancelled.\n");
            sleep(2);
            return;
        }

        if (validatePhoneNumber(phoneStr))
        {
            r.phone = atoll(phoneStr);
            phoneValid = 1;
        }
    }

    // --- Initial deposit amount ---
    while (1)
    {
        printf("\nEnter amount to deposit (or enter 0 to cancel): $");
        if (scanf("%lf", &r.amount) == 1)
        {
            if (r.amount == 0.0)
            {
                printf("\nAccount creation cancelled.\n");
                sleep(2);
                return;
            }
            if (r.amount > 0.0)
                break;
        }
        while (getchar() != '\n');
        printf("%s✖ Invalid deposit amount! Must be a positive number (or 0 to cancel).%s\n", COLOR_RED, COLOR_RESET);
    }

    // --- Account type selection ---
    int typeChoice = 0;
    while (typeChoice < 1 || typeChoice > 5)
    {
        printf("\nChoose the type of account:\n");
        printf("\t[1] Saving\n");
        printf("\t[2] Current\n");
        printf("\t[3] Fixed01 - 1 year  (4%% interest)\n");
        printf("\t[4] Fixed02 - 2 years (5%% interest)\n");
        printf("\t[5] Fixed03 - 3 years (8%% interest)\n");
        printf("\t[0] Cancel\n");
        printf("\n\tEnter your choice: ");
        if (scanf("%d", &typeChoice) != 1)
        {
            while (getchar() != '\n');
            typeChoice = -1;
        }
        if (typeChoice == 0)
        {
            printf("\nAccount creation cancelled.\n");
            sleep(2);
            return;
        }
        switch (typeChoice)
        {
            case 1: strcpy(r.accountType, "savings");  break;
            case 2: strcpy(r.accountType, "current"); break;
            case 3: strcpy(r.accountType, "fixed01"); break;
            case 4: strcpy(r.accountType, "fixed02"); break;
            case 5: strcpy(r.accountType, "fixed03"); break;
            default:
                printf("%s✖ Invalid choice. Please enter a number between 0 and 5.%s\n", COLOR_RED, COLOR_RESET);
                typeChoice = -1;
        }
    }

    // --- Format the date as a TEXT string for database storage ---
    // Stored as "MM/DD/YYYY" to match the format expected by the audit
    char dateStr[20];
    sprintf(dateStr, "%d/%d/%d", r.deposit.month, r.deposit.day, r.deposit.year);

    // --- INSERT the new account into the database ---
    // The id column is AUTOINCREMENT — SQLite assigns it automatically.
    // 'name' is omitted as it is now retrieved via standard SQL JOIN with 'users' table.
    sqlite3_prepare_v2(db,
        "INSERT INTO records (user_id, account_nbr, deposit_date, country, phone, amount, type) "
        "VALUES (?, ?, ?, ?, ?, ?, ?);",
        -1, &stmt, NULL);

    sqlite3_bind_int   (stmt, 1, u.id);              // owner's user id (foreign key)
    sqlite3_bind_int   (stmt, 2, r.accountNbr);
    sqlite3_bind_text  (stmt, 3, dateStr,      -1, SQLITE_STATIC);
    sqlite3_bind_text  (stmt, 4, r.country,    -1, SQLITE_STATIC);
    sqlite3_bind_int64 (stmt, 5, r.phone);
    sqlite3_bind_double(stmt, 6, r.amount);
    sqlite3_bind_text  (stmt, 7, r.accountType,-1, SQLITE_STATIC);

    sqlite3_step(stmt);     // execute the INSERT
    sqlite3_finalize(stmt); // free the prepared statement

    success(u);
}

// checkAllAccounts retrieves and displays a formatted list of all bank accounts belonging to the logged-in user.
void checkAllAccounts(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;
    int found = 0;

    printf("\n\t\t" COLOR_CYAN "┌──────────────────────────────────────────────┐" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "│" STYLE_BOLD COLOR_GREEN "          📋 OWNED ACCOUNTS LIST              " COLOR_CYAN "│" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "└──────────────────────────────────────────────┘" COLOR_RESET "\n");
    printf("\n\t\t" STYLE_BOLD COLOR_YELLOW "  User: %s" COLOR_RESET "\n\n", u.name);

    // Query: get every record row that belongs to this user.
    // We JOIN with the 'users' table to fetch the username dynamically.
    sqlite3_prepare_v2(db,
        "SELECT r.id, r.user_id, u.name, r.account_nbr, r.deposit_date, r.country, r.phone, r.amount, r.type "
        "FROM records r "
        "JOIN users u ON r.user_id = u.id "
        "WHERE r.user_id = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);

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

    printf("\n\t\t" COLOR_CYAN "┌──────────────────────────────────────────────┐" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "│" STYLE_BOLD COLOR_GREEN "          🔍 DETAILED ACCOUNT CHECK           " COLOR_CYAN "│" COLOR_RESET "\n");
    printf("\t\t" COLOR_CYAN "└──────────────────────────────────────────────┘" COLOR_RESET "\n\n");

    // --- First pass: show a summary list of all accounts for this user ---
    // We JOIN with the 'users' table to fetch the owner's name.
    sqlite3_prepare_v2(db,
        "SELECT r.id, r.user_id, u.name, r.account_nbr, r.deposit_date, r.country, r.phone, r.amount, r.type "
        "FROM records r "
        "JOIN users u ON r.user_id = u.id "
        "WHERE r.user_id = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);

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
        printf("Enter the account number to see full details and interest (or enter -1 to return to main menu): ");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }
    if (accNbr == -1)
    {
        return;
    }

    found = 0; // reset for the second query

    // Query: find only the specific account the user chose
    // We JOIN with the 'users' table to fetch the owner's name.
    sqlite3_prepare_v2(db,
        "SELECT r.id, r.user_id, u.name, r.account_nbr, r.deposit_date, r.country, r.phone, r.amount, r.type "
        "FROM records r "
        "JOIN users u ON r.user_id = u.id "
        "WHERE r.user_id = ? AND r.account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);
    sqlite3_bind_int(stmt, 2, accNbr);

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
        "SELECT id FROM records WHERE user_id = ? LIMIT 1;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);

    // SQLITE_ROW = at least one account was found for this user
    int found = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return found;
}

// displayUserAccounts retrieves and prints a list of all accounts owned by the user.
// Helps the user see which accounts are available before they perform operations.
void displayUserAccounts(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT account_nbr, type, amount FROM records WHERE user_id = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);

    printf("\n\t\t" COLOR_BLUE "┌──────────────────────────────────────────────┐" COLOR_RESET "\n");
    printf("\t\t" COLOR_BLUE "│" STYLE_BOLD COLOR_GREEN "            📂 YOUR AVAILABLE ACCOUNTS        " COLOR_BLUE "│" COLOR_RESET "\n");
    printf("\t\t" COLOR_BLUE "├──────────────────────────────────────────────┤" COLOR_RESET "\n");

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int accNbr = sqlite3_column_int(stmt, 0);
        const char *type = (const char *)sqlite3_column_text(stmt, 1);
        double amount = sqlite3_column_double(stmt, 2);
        printf("\t\t" COLOR_BLUE "│" COLOR_RESET "  Account #%-8d | %-8s | $%-10.2f " COLOR_BLUE "│" COLOR_RESET "\n", accNbr, type, amount);
        count++;
    }
    sqlite3_finalize(stmt);

    if (count == 0)
    {
        printf("\t\t" COLOR_BLUE "│" COLOR_RED "  (No accounts registered yet)                " COLOR_BLUE "│" COLOR_RESET "\n");
    }
    printf("\t\t" COLOR_BLUE "└──────────────────────────────────────────────┘" COLOR_RESET "\n\n");
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

    system("clear");
    displayUserAccounts(u, db);

    // Ask which account number to transact on
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number (or enter -1 to return to main menu): ");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }
    if (accNbr == -1)
    {
        return;
    }

    // --- Load the account from the database ---
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT r.id, r.user_id, u.name, r.account_nbr, r.deposit_date, r.country, r.phone, r.amount, r.type "
        "FROM records r "
        "JOIN users u ON r.user_id = u.id "
        "WHERE r.user_id = ? AND r.account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);
    sqlite3_bind_int(stmt, 2, accNbr);

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
            while (1)
            {
                printf("_____________________\n");
                printf("\nEnter amount to deposit (or enter 0 to cancel): $");
                if (scanf("%lf", &deposit) == 1)
                {
                    if (deposit == 0.0)
                    {
                        printf("\nTransaction cancelled.\n");
                        sleep(2);
                        return;
                    }
                    if (deposit > 0.0)
                        break;
                }
                while (getchar() != '\n');
                printf("%s✖ Invalid amount! Must be a positive number (or 0 to cancel).%s\n", COLOR_RED, COLOR_RESET);
            }
            r.amount += deposit; // update the local amount
            printf("_____________________\n");
            printf("New balance: $%.2f\n", r.amount);
        }
        else if (DorW == 2)
        {
            // --- Withdrawal: remove money from the account ---
            double withdraw;
            while (1)
            {
                printf("_____________________\n");
                while (1)
                {
                    printf("\nEnter amount to withdraw (or enter 0 to cancel): $");
                    if (scanf("%lf", &withdraw) == 1)
                    {
                        if (withdraw == 0.0)
                        {
                            printf("\nTransaction cancelled.\n");
                            sleep(2);
                            return;
                        }
                        if (withdraw > 0.0)
                            break;
                    }
                    while (getchar() != '\n');
                    printf("%s✖ Invalid amount! Must be a positive number (or 0 to cancel).%s\n", COLOR_RED, COLOR_RESET);
                }

                if (r.amount >= withdraw)
                {
                    r.amount -= withdraw; // update the local amount
                    printf("_____________________\n");
                    printf("New balance: $%.2f\n", r.amount);
                    break;
                }
                else
                {
                    // Overdraft protection — cannot withdraw more than the balance
                    printf("_____________________\n");
                    printf("%s\n✖ Withdrawal denied. Insufficient funds.%s\n", COLOR_RED, COLOR_RESET);
                    printf("Available balance: $%.2f\n", r.amount);
                    printf("Please enter a smaller amount.\n");
                }
            }
        }
    }
    while (DorW != 1 && DorW != 2);

    // --- Save the updated balance back to the database ---
    // Only the amount column changes — all other fields remain the same
    sqlite3_prepare_v2(db,
        "UPDATE records SET amount = ? WHERE user_id = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, r.amount);            // new balance
    sqlite3_bind_int   (stmt, 2, u.id);
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

    int accNbr;
    sqlite3_stmt *stmt;

    // Retry loop for account verification
    while (1)
    {
        system("clear");
        displayUserAccounts(u, db);

        printf("\nEnter the account number to update (or enter -1 to return to main menu): ");
        if (scanf("%d", &accNbr) != 1)
        {
            while (getchar() != '\n');
            printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
            sleep(2);
            continue;
        }

        if (accNbr == -1)
        {
            return;
        }

        // --- Verify the account exists for this user ---
        sqlite3_prepare_v2(db,
            "SELECT id FROM records WHERE user_id = ? AND account_nbr = ?;",
            -1, &stmt, NULL);
        sqlite3_bind_int(stmt, 1, u.id);
        sqlite3_bind_int(stmt, 2, accNbr);

        if (sqlite3_step(stmt) == SQLITE_ROW)
        {
            // Account found, proceed
            sqlite3_finalize(stmt);
            break;
        }
        
        // Account not found: show a clear error message, print warning and try again
        sqlite3_finalize(stmt);
        printf("%s\n✖ Error: Account #%d does not exist or does not belong to you.%s\n", COLOR_RED, accNbr, COLOR_RESET);
        printf("Please choose from one of your available accounts listed .\n");
        sleep(3);
    }

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
            char phoneStr[20];
            int phoneValid = 0;
            long long newPhone = 0;
            printf("_____________________\n");
            while (!phoneValid)
            {
                printf("\nEnter the new phone number (with country code) or enter 0 to cancel: ");
                if (scanf("%19s", phoneStr) != 1)
                {
                    while (getchar() != '\n');
                    continue;
                }
                while (getchar() != '\n');

                if (strcmp(phoneStr, "0") == 0)
                {
                    printf("\nUpdate cancelled.\n");
                    sleep(2);
                    return;
                }

                if (validatePhoneNumber(phoneStr))
                {
                    newPhone = atoll(phoneStr);
                    phoneValid = 1;
                }
            }

            // SET only the phone column — other fields unchanged
            sqlite3_prepare_v2(db,
                "UPDATE records SET phone = ? WHERE user_id = ? AND account_nbr = ?;",
                -1, &stmt, NULL);
            sqlite3_bind_int64(stmt, 1, newPhone);
            sqlite3_bind_int  (stmt, 2, u.id);
            sqlite3_bind_int  (stmt, 3, accNbr);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
        else if (choice == 2)
        {
            // --- Update country only ---
            char newCountry[100];
            int code = -1;
            printf("_____________________\n");
            while (code == -1)
            {
                printf("\nEnter the new country (or enter 0 to cancel): ");
                // Use a width limit of 99 to prevent buffer overflow on the 100-byte newCountry buffer.
                scanf("%99s", newCountry);
                if (strcmp(newCountry, "0") == 0)
                {
                    printf("\nUpdate cancelled.\n");
                    sleep(2);
                    return;
                }
                code = getCountryCode(newCountry);
                if (code == -1)
                    printf("%s✖ Country '%s' not recognized. Please try again.%s\n", COLOR_RED, newCountry, COLOR_RESET);
            }

            // SET only the country column — other fields unchanged
            sqlite3_prepare_v2(db,
                "UPDATE records SET country = ? WHERE user_id = ? AND account_nbr = ?;",
                -1, &stmt, NULL);
            sqlite3_bind_text(stmt, 1, newCountry, -1, SQLITE_STATIC);
            sqlite3_bind_int (stmt, 2, u.id);
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

    system("clear");
    displayUserAccounts(u, db);

    // Ask which account to delete
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number (or enter -1 to return to main menu): ");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }
    if (accNbr == -1)
    {
        return;
    }

    // DELETE the row matching this user + account number
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "DELETE FROM records WHERE user_id = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);
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

    system("clear");
    displayUserAccounts(u, db);

    // Ask which account to transfer
    int accNbr;
    while (1)
    {
        printf("\nEnter the account number (or enter -1 to return to main menu): ");
        if (scanf("%d", &accNbr) == 1)
            break;
        while (getchar() != '\n');
        printf("%s✖ Invalid account number! Please enter digits only.%s\n", COLOR_RED, COLOR_RESET);
    }
    if (accNbr == -1)
    {
        return;
    }

    // --- Verify the account exists and belongs to this user ---
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE user_id = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, u.id);
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
    // Use a width limit of 99 to prevent buffer overflow on the 100-byte newUserName buffer.
    scanf("%99s", newUserName);

    // Prevent user from transferring an account to themselves
    if (strcmp(newUserName, u.name) == 0)
    {
        printf("%s\n✖ Transfer failed! You cannot transfer an account to yourself.%s\n", COLOR_RED, COLOR_RESET);
        stayOrReturn(1, transferOwner, u, db);
        return;
    }

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

    // --- Verify the recipient doesn't already own an account with this number ---
    // We query by recipient user ID and account number to make sure they do not already have it.
    sqlite3_prepare_v2(db,
        "SELECT id FROM records WHERE user_id = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, newUserId);
    sqlite3_bind_int(stmt, 2, accNbr);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Recipient already owns an account with this number.
        sqlite3_finalize(stmt);
        printf("%s\n✖ Transfer failed! The recipient '%s' already owns an account with number %d.%s\n", COLOR_RED, newUserName, accNbr, COLOR_RESET);
        stayOrReturn(1, transferOwner, u, db);
        return;
    }
    sqlite3_finalize(stmt);

    // --- Update the record: change only the user_id to the new owner ---
    // We no longer update 'name' because the 'records' table is normalized.
    sqlite3_prepare_v2(db,
        "UPDATE records SET user_id = ? WHERE user_id = ? AND account_nbr = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_int(stmt, 1, newUserId); // new owner's ID
    sqlite3_bind_int(stmt, 2, u.id);      // current owner's ID
    sqlite3_bind_int(stmt, 3, accNbr);     // account being transferred
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // --- Send the transfer notification instantly via FIFO pipe ---
    char target_fifo[150];
    snprintf(target_fifo, sizeof(target_fifo), "./data/fifo_%s", newUserName);

    // Open the recipient's FIFO in non-blocking write mode so we don't hang if they are offline
    int fd = open(target_fifo, O_WRONLY | O_NONBLOCK);
    if (fd != -1)
    {
        char notifMsg[300];
        snprintf(notifMsg, sizeof(notifMsg), "Account #%d has been transferred to you by %s.", accNbr, u.name);
        write(fd, notifMsg, strlen(notifMsg));
        close(fd);
    }

    success(u);
}
