#include "header.h"

// RECORDS is the default file path for storing all account records.
const char *RECORDS = "./data/records.txt";

// getAccountFromFile reads a single account record from the provided file pointer into the Record struct.
// It returns 1 if a record was successfully read, or 0 if it reached the end of the file (EOF).
int getAccountFromFile(FILE *ptr, char name[50], struct Record *r)
{
    return fscanf(ptr, "%d %d %s %d %d/%d/%d %s %lld %lf %s",
                  &r->id,
		  &r->userId,
		  name,
                  &r->accountNbr,
                  &r->deposit.month,
                  &r->deposit.day,
                  &r->deposit.year,
                  r->country,
                  &r->phone,
                  &r->amount,
                  r->accountType) != EOF;
}

// saveAccountToFile writes the details of a single Record struct to the provided file pointer.
// It ensures the data is formatted consistently with spacing and newlines for future retrieval.
void saveAccountToFile(FILE *ptr, struct Record r)
{
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %lld %.2lf %s\n\n",
            r.id,
	        r.userId,
	        r.name,
            r.accountNbr,
            r.deposit.month,
            r.deposit.day,
            r.deposit.year,
            r.country,
            r.phone,
            r.amount,
            r.accountType);
}

// stayOrReturn prompts the user to either retry the current operation, return to the main menu, or exit the program.
// It handles cases where an account is not found (notGood == 0) or simply serves as a navigation prompt.
void stayOrReturn(int notGood, void f(struct User u), struct User u)
{
    int option;
    if (notGood == 0)
    {
        system("clear");
        printf("%s\n✖ Oops! We couldn't find a record matching that account.%s\n", COLOR_RED, COLOR_RESET);
    invalid:
        printf("\nEnter 0 to try again, 1 to return to main menu and 2 to exit:");
        if (scanf("%d", &option) != 1) {
            while (getchar() != '\n');
            option = -1;
        }
        if (option == 0)
        {
            f(u);
            return;
        }    
        else if (option == 1)
        { 
            mainMenu(u);
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
        mainMenu(u);
    }
    else
    {
        system("clear");
        exit(1);
    }
}

// success displays a positive confirmation message and prompts the user to navigate back to the menu or exit.
// It prevents the terminal from immediately closing or clearing before the user has a chance to read the result.
void success(struct User u)
{
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
        mainMenu(u);
    }
    else if (option == 0)
    {
        exit(1);
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

// createNewAcc prompts the user to input details for a new bank account and appends it to the records file.
// It prevents duplicate account numbers for the same user and initializes the account balance.
void createNewAcc(struct User u)
{
    struct Record r;
    struct Record cr;
    char userName[50];
    int lastId = -1;
    time_t t = time(NULL);
    struct tm *today = localtime(&t);
    FILE *pf = fopen(RECORDS, "a+");

noAccount:
    system("clear");
    printf("\t\t\t===== New record =====\n");

    printf("\nEnter today's date(mm/dd/yyyy):");
    scanf("%d/%d/%d", &r.deposit.month, &r.deposit.day, &r.deposit.year);
    if (r.deposit.month != (today->tm_mon + 1) ||
        r.deposit.day   != today->tm_mday       ||
        r.deposit.year  != (today->tm_year + 1900))
    {     printf("%s✖ Please enter today's date only!%s\n", COLOR_RED, COLOR_RESET);
          sleep(3);
          goto noAccount;
    
    }else
    {
    printf("\nEnter the account number:");
    scanf("%d", &r.accountNbr);

    rewind(pf); // Reset file pointer to beginning before reading for account number
    while (getAccountFromFile(pf, userName, &cr))
    {
        if (strcmp(userName, u.name) == 0 && cr.accountNbr == r.accountNbr)
        {
            printf("%s✖ This account number is already in use by this user. Please try a different one.%s\n\n", COLOR_RED, COLOR_RESET);
            goto noAccount;
        }
        lastId = cr.id;
    }
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
    char phoneStr[20];
    int phoneValid = 0;
    while (!phoneValid)
    {
        printf("\nEnter phone number (with country code e.g. %d...): ", code);
        scanf("%19s", phoneStr);
        while (getchar() != '\n'); // Clear leftover input from buffer

        int len = strlen(phoneStr);

        // Check all characters are digits
        int digitsOnly = 1;
        for (int i = 0; i < len; i++)
        {
            if (phoneStr[i] < '0' || phoneStr[i] > '9')
            {
                digitsOnly = 0;
                break;
            }
        }

        // Check length is within international standard (8-15 digits)
        if (!digitsOnly)
            printf("%s✖ Phone number must contain digits only (no spaces or dashes).%s\n", COLOR_RED, COLOR_RESET);
        else if (len < 8 || len > 15)
            printf("%s✖ Phone number must be between 8 and 15 digits.%s\n", COLOR_RED, COLOR_RESET);
        else
        {
            // Check it starts with the country code
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
    printf("\nEnter amount to deposit: $");
    scanf("%lf", &r.amount);
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
            case 1: strcpy(r.accountType, "saving");  break;
            case 2: strcpy(r.accountType, "current"); break;
            case 3: strcpy(r.accountType, "fixed01"); break;
            case 4: strcpy(r.accountType, "fixed02"); break;
            case 5: strcpy(r.accountType, "fixed03"); break;
            default:
                printf("%s✖ Invalid choice. Please enter a number between 1 and 5.%s\n", COLOR_RED, COLOR_RESET);
                typeChoice = 0;
        }
    }

    r.userId = u.id;
    r.id = lastId + 1;
    strcpy(r.name, u.name);
    saveAccountToFile(pf, r);

    fclose(pf);
    success(u);
    }  
}

// checkAllAccounts retrieves and displays a formatted list of all bank accounts associated with the logged-in user.
// It iterates through the entire records file and filters by the user's name.
void checkAllAccounts(struct User u)
{
    char userName[100];
    struct Record r;
    int found = 0;

    FILE *pf = fopen(RECORDS, "r");
    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            found = 1;
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:+%lld \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.month,
                   r.deposit.day,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
        }
    }
    fclose(pf);
    if (!found)
    {
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, checkAllAccounts, u);
        return;
    }
    success(u);
}

// checkAcc first lists all accounts belonging to the user, then prompts for an account number.
// It displays the full details and calculates the expected interest yield based on the account type.
void checkAcc(struct User u)
{
    char userName[100];
    struct Record r;
    int found = 0;
    double interest;

    FILE *pf = fopen(RECORDS, "r");

    // First, list all accounts belonging to the user
    system("clear");
    printf("\t\t====== Your accounts =====\n\n");
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            found = 1;
            printf("_____________________\n");
            printf("\nAccount number: %d | Type: %s | Balance: $%.2f\n",
                   r.accountNbr, r.accountType, r.amount);
        }
    }

    if (!found)
    {
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        fclose(pf);
        stayOrReturn(1, checkAcc, u);
        return;
    }

    // Then ask the user to pick one
    int accNbr;
    printf("\n_____________________\n");
    printf("Enter the account number to see full details and interest: ");
    scanf("%d", &accNbr);

    // Rewind and find the chosen account
    rewind(pf);
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:+%lld \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.month,
                   r.deposit.day,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
            found = 1;

            // Calculate and display monthly interest based on the account type
            if (strcmp(r.accountType, "savings") == 0)
            {
                interest = (r.amount * 0.07) / 12;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on day %d of every month\n", interest, r.deposit.day);
            }
            else if (strcmp(r.accountType, "fixed01") == 0)
            {
                interest = r.amount * 0.04;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 1);
            }
            else if (strcmp(r.accountType, "fixed02") == 0)
            {
                interest = r.amount * 0.05;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 2);
            }
            else if (strcmp(r.accountType, "fixed03") == 0)
            {
                interest = r.amount * 0.08;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on %d/%d/%d\n", interest, r.deposit.month, r.deposit.day, r.deposit.year + 3);
            }
            else if (strcmp(r.accountType, "current") == 0)
            {
                printf("_____________________\n");
                printf("You will not get interests because the account is of type current\n");
            }
        }
    }
    fclose(pf);
    if (!found)
        stayOrReturn(0, checkAcc, u);
    success(u);
}

// userHasAccounts checks if the logged-in user has any accounts in the records file.
// Returns 1 if at least one account is found, 0 otherwise.
int userHasAccounts(struct User u)
{
    FILE *pf = fopen(RECORDS, "r");
    char userName[100];
    struct Record r;
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            fclose(pf);
            return 1;
        }
    }
    fclose(pf);
    return 0;
}

// makeTransaction handles depositing and withdrawing funds for a given account.
// It validates that the account is not a fixed-term deposit (which rejects transactions) and prevents overdrafts.
void makeTransaction(struct User u)
{
    if (!userHasAccounts(u))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, makeTransaction, u);
        return;
    }
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    struct Record r;
    int found = 0;
    int DorW;
    double deposit;
    double withdraw;



    FILE *pf = fopen(RECORDS, "r");
    FILE  *temp = fopen("temp.txt", "w");
    // Iterate through all records, copying them to a temp file, modifying only the target record
    while (getAccountFromFile(pf, userName, &r))
    {

    if (strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            // Reject transactions for fixed-term deposit accounts
            if (strcmp(r.accountType, "fixed01") == 0 || strcmp(r.accountType, "fixed02") == 0 || strcmp(r.accountType, "fixed03") == 0)
            {
                printf("_____________________\n");
                printf("%s\n✖ Transactions (deposits/withdrawals) are not allowed for fixed-term accounts.%s\n", COLOR_RED, COLOR_RESET);
                saveAccountToFile(temp, r);
                found = 1;
            }
            else
            {
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
                printf("_____________________\n");
                printf("\nEnter amount to deposit: $");
                scanf("%lf", &deposit);
                r.amount += deposit;
                printf("_____________________\n");
                printf("New balance: $%.2f\n", r.amount);
                saveAccountToFile(temp, r);
            }
            if (DorW == 2)
            {   
                printf("_____________________\n");
                printf("\nEnter amount to withdraw: $");
                scanf("%lf", &withdraw);
                if (r.amount >= withdraw)
                {
                    r.amount -= withdraw;
                    printf("_____________________\n");
                    printf("New balance: $%.2f\n", r.amount);
                    saveAccountToFile(temp, r);
                } else
                {   
                    printf("_____________________\n");
                    printf("%s\n✖ Withdrawal denied. Insufficient funds.%s\n", COLOR_RED, COLOR_RESET);
                    printf("Available balance: $%.2f\n", r.amount);
                    saveAccountToFile(temp,r);
                }
            } 
            }
            while (DorW != 1 && DorW != 2);
            found = 1;
            }
        } else
        {
            saveAccountToFile(temp, r);
        }
        
    }
    fclose(pf);
    fclose(temp);
    
    // Replace the old records file with the updated temporary file
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, makeTransaction, u);
    success(u);
}    


// updateAcc allows the user to modify the phone number or country associated with a specific account.
// It rewrites the records file, replacing the old account details with the updated values.
void updateAcc(struct User u)
{

    if (!userHasAccounts(u))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, updateAcc, u);
        return;
    }
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    struct Record r;
    int found = 0;

    FILE *pf = fopen(RECORDS, "r");
    FILE *temp = fopen("temp.txt", "w");
    while (getAccountFromFile(pf, userName, &r))
    {
        if(strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            do
            {
                printf("_____________________\n");
                printf("What do you want to update?\n");
                printf("Phone number:1  or country:2 ?");
                if (scanf("%d", &found) != 1) {
                    while (getchar() != '\n');
                    found = -1;
                }
                if (found == 1)
                {
                    printf("_____________________\n");
                    printf("\nEnter the new phone number:");
                    scanf("%d", &r.phone);
                saveAccountToFile(temp, r);
                }
                else if (found == 2)
                {
                    printf("_____________________\n");
                    printf("\nEnter the new country:");
                    scanf("%s", r.country);
                saveAccountToFile(temp, r);
                }
            } while(found != 1 && found != 2);
        } 
        else
        {
        saveAccountToFile(temp, r);  
        }
    }
    fclose(pf);
    fclose(temp);
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, updateAcc, u);
        success(u);

}

// removeAcc deletes a specific account from the system.
// It accomplishes this by copying every other account to a temporary file, skipping the target account, and then replacing the original file.
void removeAcc(struct User u)
{
    if (!userHasAccounts(u))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, removeAcc, u);
        return;
    }
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    struct Record r;
    int found = 0;

    FILE *pf = fopen(RECORDS, "r");
    FILE *temp = fopen("temp.txt", "w");
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            found = 1;
        }
        else
        {
            saveAccountToFile(temp, r);
        }
    }   
    fclose(pf);
    fclose(temp);
    
    // Replace the old records file with the updated temporary file
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, removeAcc, u);
    success(u);
}    

// transferOwner reassigns the ownership of a given account to another registered user.
// It verifies the target user's existence in the system and updates both the account name and user ID securely.
void transferOwner(struct User u)
{
    if (!userHasAccounts(u))
    {
        system("clear");
        printf("%s\nYou have no accounts yet. Create one from the main menu!%s\n", COLOR_YELLOW, COLOR_RESET);
        stayOrReturn(1, transferOwner, u);
        return;
    }
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    char newUserName[100];
    struct Record r;
    int found = 0;
    int checkId;
    char checkName[100];
    char checkPass[100];
   

    FILE *pf= fopen(RECORDS, "r");
    FILE *temp = fopen("temp.txt", "w");
    while (getAccountFromFile(pf, userName, &r))
    {
        if(strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            printf("_____________________\n");
            printf("\nEnter the new owner name:");
            scanf("%s", newUserName);
            
           // Verify the target user exists and fetch their ID before transferring ownership
           FILE *pf2 =fopen("./data/users.txt", "r");
           while(fscanf(pf2, "%d %s %s", &checkId, checkName, checkPass) != EOF)
           {
               if(strcmp(checkName, newUserName) == 0)
               {
                   strcpy(r.name, newUserName);
                   r.userId = checkId;
                   found = 1;
               }   
           }
           fclose(pf2);
           saveAccountToFile(temp, r);
        } else
        {
            saveAccountToFile(temp, r);
        }
    }      
    fclose(pf);
    fclose(temp);
    
    // Replace the old records file with the updated temporary file
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, transferOwner, u);
    success(u);
    }



        









    

    
