#include "header.h"

// Globals to manage the background notification child process and its FIFO pipe
static pid_t notification_child_pid = -1;
static char fifo_path[150] = "";

// stopNotificationListener kills the child background listener and unlinks the FIFO file
void stopNotificationListener()
{
    if (notification_child_pid > 0)
    {
        kill(notification_child_pid, SIGTERM);
        notification_child_pid = -1;
    }
    if (fifo_path[0] != '\0')
    {
        unlink(fifo_path);
        fifo_path[0] = '\0';
    }
}

// startNotificationListener forks a background child process that blocks on the user's FIFO
// and prints any incoming transfer notifications directly to the terminal instantly.
void startNotificationListener(struct User u)
{
    // Clean up any old listener first
    stopNotificationListener();

    // Construct the path for this user's named pipe
    snprintf(fifo_path, sizeof(fifo_path), "./data/fifo_%s", u.name);

    // Create the FIFO (named pipe) with read/write permissions
    mkfifo(fifo_path, 0666);

    // Fork a child process to listen to the FIFO in the background
    notification_child_pid = fork();
    if (notification_child_pid < 0)
    {
        // Fork failed
        return;
    }
    else if (notification_child_pid == 0)
    {
        // --- Child Process ---
        char buffer[300];
        while (1)
        {
            // Open the FIFO for reading (blocks until a writer opens it)
            int fd = open(fifo_path, O_RDONLY);
            if (fd == -1)
            {
                sleep(1);
                continue;
            }

            // Read the message from the writer
            ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);
            if (bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                // Print the notification directly to the shared stdout of the terminal
                printf("\n\n%s📬 Instant Notification: %s%s\n", COLOR_GREEN, buffer, COLOR_RESET);
                fflush(stdout);
            }
            close(fd);
        }
        exit(0);
    }

    // --- Parent Process ---
    // Register the cleanup function to automatically run when the parent exits
    atexit(stopNotificationListener);
}

// mainMenu displays the primary ATM operations available to an authenticated user and handles their selection.
// It routes the user to various system functions like creating accounts, making transactions, and checking balances.
void mainMenu(struct User u,sqlite3 *db)
{
    int option;
    while (1)
    {
        system("clear");
        printf("\n\n\t\t======= ATM =======\n\n");
        printf("\n\t\t-->> Feel free to choose one of the options below <<--\n");
        printf("\n\t\t[1]- Create a new account\n");
        printf("\n\t\t[2]- Update account information\n");
        printf("\n\t\t[3]- Check accounts\n");
        printf("\n\t\t[4]- Check list of owned account\n");
        printf("\n\t\t[5]- Make Transaction\n");
        printf("\n\t\t[6]- Remove existing account\n");
        printf("\n\t\t[7]- Transfer ownership\n");
        printf("\n\t\t[8]- Exit\n");
        
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n'); // Clear the input buffer
            option = -1; // Set to invalid option
        }

        switch (option)
        {
        case 1:
            createNewAcc(u, db);
            break;
        case 2:
            updateAcc(u, db);
            break;
        case 3:
            checkAcc(u, db);
            break;
        case 4:
            checkAllAccounts(u, db);
            break;
        case 5:
            makeTransaction(u, db);
            break;
        case 6:
            removeAcc(u, db);
            break;
        case 7:
            transferOwner(u, db);
            break;
        case 8:
            exit(0);
            break;
        default:
            printf("%sInvalid operation! Please choose a valid option.%s\n", COLOR_RED, COLOR_RESET);
            sleep(2);
            break;
        }
    }
}

// initMenu displays the initial welcome screen, allowing users to log in or register a new account securely.
// It manages password encryption, ensures usernames are unique during registration, and assigns sequential user IDs.
void initMenu(struct User *u,sqlite3 *db)
{
    int r = 0;
    int option;
    while (!r)
    {
        system("clear");
        printf("\n\n\t\t======= ATM =======\n");
        printf("\n\t\t-->> Feel free to login / register :\n");
        printf("\n\t\t[1]- login\n");
        printf("\n\t\t[2]- register\n");
        printf("\n\t\t[3]- exit\n");
        printf("\n\t\tEnter option: ");
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n'); // Clear the input buffer
            option = -1; // Set to invalid option
        }
        switch (option)
        {
        case 1:
            login:
            loginMenu(u->name, u->password);
            
            // Retrieve the password from the database.
            // If the user doesn't exist, getPassword returns NULL.
            const char *dbPassword = getPassword(*u, db);
            
            // Verify that the user exists (dbPassword is not NULL)
            // and that the entered password matches the stored password.
            if (dbPassword != NULL && strcmp(u->password, dbPassword) == 0)
            {
                // --- Retrieve the user's ID from the database ---
                // We need the ID so the rest of the app knows which user is logged in.
                // Without it, account operations would link to the wrong user.
                sqlite3_stmt *stmt;

                // Prepare a query to find the id of the logged-in user by name
                sqlite3_prepare_v2(db,
                    "SELECT id FROM users WHERE name = ?;",
                    -1, &stmt, NULL);

                // Bind the username as the search value for the ? placeholder
                sqlite3_bind_text(stmt, 1, u->name, -1, SQLITE_STATIC);

                // Execute — SQLITE_ROW means the user was found
                if (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    // Column 0 is the id — store it in the User struct
                    u->id = sqlite3_column_int(stmt, 0);
                }

                // Always finalize the statement to free SQLite's internal memory
                sqlite3_finalize(stmt);

                printf("%s\n\n✔ Password Match! Welcome back.%s\n", COLOR_GREEN, COLOR_RESET);
                sleep(2);
            }
            else
            {
                printf("%s\n✖ Login failed! Incorrect username or password.%s\n", COLOR_RED, COLOR_RESET);
                sleep(2);
                goto login;
            }
            r = 1;
            break;
        case 2:
            // --- REGISTRATION ---
            // Keep asking for a username until we get one that doesn't exist yet
            {
                int cancelled = 0;
                while (1)
                {
                    printf("\n\n\t\"Type in a new Username (or enter 0 to return to welcome menu): ");
                    // Use a width limit of 49 to prevent buffer overflow on the 50-byte username buffer.
                    scanf("%49s", u->name);
                    while (getchar() != '\n'); // Clear the newline left in the buffer

                    if (strcmp(u->name, "0") == 0)
                    {
                        cancelled = 1;
                        break;
                    }

                    // --- Check if username already exists in the database ---
                    sqlite3_stmt *stmt;

                    // Query: look for any user with this exact name
                    sqlite3_prepare_v2(db,
                        "SELECT id FROM users WHERE name = ?;",
                        -1, &stmt, NULL);

                    // Bind the typed username as the search value
                    sqlite3_bind_text(stmt, 1, u->name, -1, SQLITE_STATIC);

                    // SQLITE_ROW = a row was found = username already taken
                    int nameExists = (sqlite3_step(stmt) == SQLITE_ROW);

                    // Always clean up the statement after use
                    sqlite3_finalize(stmt);

                    if (nameExists)
                    {
                        printf("%s\n✖ Registration failed! A user with this name already exists. Please try another username (or enter 0 to return).%s\n", COLOR_RED, COLOR_RESET);
                    }
                    else
                    {
                        break; // Username is unique — exit the loop
                    }
                }

                if (cancelled)
                {
                    break; // Break the switch case to return to welcome option selection loop
                }
            }

            // --- Get and encrypt the password ---
            printf("\n\t\"Type in a new Password: ");
            getMaskedPassword(u->password);

            // Encrypt using Caesar cipher: shift each character forward by +3
            // This means 'a' becomes 'd', 'z' becomes '}', etc.
            int p_idx = 0;
            while (u->password[p_idx] != '\0')
            {
                u->password[p_idx] = u->password[p_idx] + 3;
                p_idx++;
            }

            // --- Insert the new user into the database ---
            {
                sqlite3_stmt *stmt;

                // The id column is INTEGER PRIMARY KEY so SQLite auto-assigns it.
                // We only need to provide name and (encrypted) password.
                sqlite3_prepare_v2(db,
                    "INSERT INTO users (name, password) VALUES (?, ?);",
                    -1, &stmt, NULL);

                // Bind the username to the first ?
                sqlite3_bind_text(stmt, 1, u->name,     -1, SQLITE_STATIC);
                // Bind the encrypted password to the second ?
                sqlite3_bind_text(stmt, 2, u->password, -1, SQLITE_STATIC);

                // Execute the INSERT
                sqlite3_step(stmt);

                // Retrieve the auto-assigned id so the session knows who is logged in
                // sqlite3_last_insert_rowid returns the id of the row just inserted
                u->id = (int)sqlite3_last_insert_rowid(db);

                // Clean up
                sqlite3_finalize(stmt);
            }

            r = 1;
            break;

        case 3:
            exit(1);
            break;
        default:
            printf("%sInvalid operation! Please choose a valid option.%s\n", COLOR_RED, COLOR_RESET);
        }
    }
};

// main is the entry point of the ATM Management System, initializing the session and launching the interface.
int main()
{
    struct User u;
    sqlite3 *db=initDB();
    initMenu(&u,db);
    
    // Start background IPC notification listener for this session
    extern void startNotificationListener(struct User u);
    startNotificationListener(u);

    mainMenu(u,db);
    sqlite3_close(db);
    return 0;
}
