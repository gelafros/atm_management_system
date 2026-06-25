#include <termios.h>
#include "header.h"

char *USERS = "./data/users.txt";

// getMaskedPassword reads user input from the terminal without echoing it, displaying asterisks instead.
// It temporarily disables canonical mode and terminal echo to securely capture the password.
void getMaskedPassword(char *password)
{
    struct termios oldt, newt;
    int i = 0;
    int c;

    tcgetattr(fileno(stdin), &oldt);
    newt = oldt;
    // Disable canonical mode (buffered output) and local echo
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &newt);

    while ((c = getchar()) != '\n' && c != '\r' && c != EOF)
    {
        if (c == 127 || c == '\b') // Handle Backspace
        {
            if (i > 0)
            {
                i--;
                printf("\b \b"); // Move cursor back, print space to erase, move cursor back
            }
        }
        else if (i < 49) // Leave room for the null terminator '\0'
        {
            password[i++] = c;
            printf("*");
        }
    }
    password[i] = '\0';
    printf("\n");

    tcsetattr(fileno(stdin), TCSANOW, &oldt);
}

// loginMenu prompts the user to enter their username and securely input their password.
// It clears the terminal before displaying the bank management system login interface.
void loginMenu(char a[50], char pass[50])
{
    system("clear");
    printf("\n\n\n\t\t\t\t   Bank Management System\n\t\t\t\t\t User Login:");
    // Use a width limit of 49 to prevent buffer overflow on the 50-byte username buffer.
    scanf("%49s", a);
    while (getchar() != '\n'); // Clear the newline left in the buffer by scanf

    printf("\n\n\n\n\n\t\t\t\tEnter the password to login:");
    getMaskedPassword(pass);
}

// getPassword queries the SQLite database for the stored (encrypted) password
// of the user whose name matches u.name.
// It decrypts it using Caesar cipher (shift -3) and returns it as a static string.
// Returns "no user found" if the username does not exist in the database.
const char *getPassword(struct User u, sqlite3 *db)
{
    // stmt will hold our prepared SQL query
    sqlite3_stmt *stmt;

    // static buffer to hold the decrypted password across the function return
    // (must be static so it stays alive after the function returns)
    static char decrypted[50];

    // Step 1: Prepare the SQL query with a ? placeholder for the username.
    // This is safer than building the string directly (prevents SQL injection).
    sqlite3_prepare_v2(db,
        "SELECT password FROM users WHERE name = ?;",
        -1, &stmt, NULL);

    // Step 2: Fill in the ? placeholder with the actual username we are looking for
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    // Step 3: Execute the query.
    // SQLITE_ROW means a matching row was found in the database.
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        // Step 4: Read column 0 (the password column) from the result row
        const char *pass = (const char *)sqlite3_column_text(stmt, 0);

        // Copy into our static buffer so we can safely return it
        strcpy(decrypted, pass);

        // Step 5: Clean up the statement — always required after use
        sqlite3_finalize(stmt);

        // Decrypt the password by shifting each character back by -3
        // (reverses the Caesar cipher +3 encryption applied during registration)
        int i = 0;
        while (decrypted[i] != '\0')
        {
            decrypted[i] = decrypted[i] - 3;
            i++;
        }

        return decrypted;
    }

    // No matching user was found in the database.
    // Instead of returning the sentinel string "no user found", we return NULL.
    // The calling function must check for NULL to avoid segfaults and prevent unauthorized login.
    sqlite3_finalize(stmt);
    return NULL;
}