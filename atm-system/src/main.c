#include "header.h"

// mainMenu displays the primary ATM operations available to an authenticated user and handles their selection.
// It routes the user to various system functions like creating accounts, making transactions, and checking balances.
void mainMenu(struct User u)
{
    int option;
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
        createNewAcc(u);
        break;
    case 2:
        // student TODO : add your **Update account information** function
        // here
        updateAcc(u);
        break;
    case 3:
        // student TODO : add your **Check the details of existing accounts** function
        // here
        checkAcc(u);
        break;
    case 4:
        checkAllAccounts(u);
        break;
    case 5:
        // student TODO : add your **Make transaction** function
        // here
        makeTransaction(u);
        break;
    case 6:
        // student TODO : add your **Remove existing account** function
        // here
        removeAcc(u);
        break;
    case 7:
        // student TODO : add your **Transfer owner** function
        // here
        transferOwner(u);
        break;
    case 8:
        exit(1);
        break;
    default:
        printf("%sInvalid operation! Please choose a valid option.%s\n", COLOR_RED, COLOR_RESET);
        mainMenu(u); // Prompt again instead of closing the program
    }
};

// initMenu displays the initial welcome screen, allowing users to log in or register a new account securely.
// It manages password encryption, ensures usernames are unique during registration, and assigns sequential user IDs.
void initMenu(struct User *u)
{
    int r = 0;
    int option;
    system("clear");
    printf("\n\n\t\t======= ATM =======\n");
    printf("\n\t\t-->> Feel free to login / register :\n");
    printf("\n\t\t[1]- login\n");
    printf("\n\t\t[2]- register\n");
    printf("\n\t\t[3]- exit\n");
    while (!r)
    {
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n'); // Clear the input buffer
            option = -1; // Set to invalid option
        }
        switch (option)
        {
        case 1:
            login:
            loginMenu(u->name, u->password);
            if (strcmp(u->password, getPassword(*u)) == 0)
            {
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
            // student TODO : add your **Registration** function
            // here
            while (1)
            {
                printf("\n\n\t\"Type in a new Username: ");
                scanf("%s", u->name);
                while (getchar() != '\n'); // Clear the newline left in the buffer

                FILE *file = fopen("./data/users.txt", "r");
                int nameExists = 0;
                if (file != NULL)
                {
                    int checkId;
                    char checkName[100];
                    char checkPass[100];
                    while (fscanf(file, "%d %s %s", &checkId, checkName, checkPass) != EOF)
                    {
                        if (strcmp(checkName, u->name) == 0)
                        {
                            nameExists = 1;
                            break;
                        }
                    }
                    fclose(file);
                }

                if (nameExists)
                {
                    printf("%s\n✖ Registration failed! A user with this name already exists. Please try another username.%s\n", COLOR_RED, COLOR_RESET);
                }
                else
                {
                    break; // Username is unique
                }
            }

            printf("\n\t\"Type in a new Password: ");
            getMaskedPassword(u->password);
            
            // Encrypt the password using a Caesar cipher (shift by +3) before saving
            int p_idx = 0;
            while (u->password[p_idx] != '\0')
            {
                u->password[p_idx] = u->password[p_idx] + 3;
                p_idx++;
            }
            
            FILE *file = fopen("./data/users.txt", "a+");
            if (file == NULL)
            {
                printf("Error opening file!");
                exit(1);
            }
                
            // Scan the existing users to find the highest ID
            int lastID = -1;
            char checkName[100];
            char checkPass[100];
            int checkId;
            rewind(file);
            while ((fscanf(file, "%d %s %s", &checkId, checkName, checkPass) != EOF))
            {
                lastID = checkId;
            }
            
            // Auto-increment the new user's ID based on the last found ID
            u->id = lastID + 1;
                
            // Append the fully formed user record to the file
            fprintf(file, "%d %s %s\n", u->id, u->name, u->password);
            fclose(file);

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
    
    initMenu(&u);
    mainMenu(u);
    return 0;
}
