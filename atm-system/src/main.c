#include "header.h"

void mainMenu(struct User u)
{
    // Displays the main operations available to a logged-in user
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
    scanf("%d", &option);

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
    }
};

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
        scanf("%d", &option);
        switch (option)
        {
        case 1:
            loginMenu(u->name, u->password);
            if (strcmp(u->password, getPassword(*u)) == 0)
            {
                printf("%s\n\n✔ Password Match! Welcome back.%s\n", COLOR_GREEN, COLOR_RESET);
            }
            else
            {
                printf("%s\n✖ Login failed! Incorrect username or password.%s\n", COLOR_RED, COLOR_RESET);
                exit(1);
            }
            r = 1;
            break;
        case 2:
            // student TODO : add your **Registration** function
            // here
            printf("\n\n\t\"Type in a new Username: ");
            scanf("%s", u->name);
            while (getchar() != '\n'); // Clear the newline left in the buffer
            printf("\n\t\"Type in a new Password: ");
            getMaskedPassword(u->password);
            FILE *file = fopen("./data/users.txt", "a+");
                
                // Scan the existing users to ensure uniqueness and find the highest ID
                int lastID=-1;
                char checkName[100];
                char checkPass[100];
                int checkId;
                while((fscanf(file, "%d %s %s", &checkId, checkName, checkPass) != EOF))
                {
                    if(strcmp(checkName, u->name) == 0)
                    {
                        printf("%s\n✖ Registration failed! A user with this name already exists.%s\n", COLOR_RED, COLOR_RESET);
                        exit(1);
                    }else{   
                    lastID = checkId;
                    }
                }
                
                // Auto-increment the new user's ID based on the last found ID
                u->id = lastID + 1;
                
            // Append the fully formed user record to the file
            fprintf(file, "%d %s %s\n",u->id, u->name, u->password);
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

int main()
{
    struct User u;
    
    initMenu(&u);
    mainMenu(u);
    return 0;
}
