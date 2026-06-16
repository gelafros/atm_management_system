#include "header.h"

const char *RECORDS = "./data/records.txt";

int getAccountFromFile(FILE *ptr, char name[50], struct Record *r)
{
    return fscanf(ptr, "%d %d %s %d %d/%d/%d %s %d %lf %s",
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

void saveAccountToFile(FILE *ptr, struct Record r)
{
    fprintf(ptr, "%d %d %s %d %d/%d/%d %s %d %.2lf %s\n\n",
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

void stayOrReturn(int notGood, void f(struct User u), struct User u)
{
    int option;
    if (notGood == 0)
    {
        system("clear");
        printf("\n✖ Record not found!!\n");
    invalid:
        printf("\nEnter 0 to try again, 1 to return to main menu and 2 to exit:");
        scanf("%d", &option);
        if (option == 0)
            f(u);
        else if (option == 1)
            mainMenu(u);
        else if (option == 2)
            exit(0);
        else
        {
            printf("Insert a valid operation!\n");
            goto invalid;
        }
    }
    else
    {
        printf("\nEnter 1 to go to the main menu and 0 to exit:");
        scanf("%d", &option);
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

void success(struct User u)
{
    int option;
    printf("\n✔ Success!\n\n");
invalid:
    printf("Enter 1 to go to the main menu and 0 to exit!\n");
    scanf("%d", &option);
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
        printf("Insert a valid operation!\n");
        goto invalid;
    }
}

void createNewAcc(struct User u)
{
    struct Record r;
    struct Record cr;
    char userName[50];
    FILE *pf = fopen(RECORDS, "a+");

noAccount:
    system("clear");
    printf("\t\t\t===== New record =====\n");

    printf("\nEnter today's date(mm/dd/yyyy):");
    scanf("%d/%d/%d", &r.deposit.month, &r.deposit.day, &r.deposit.year);
    printf("\nEnter the account number:");
    scanf("%d", &r.accountNbr);

    while (getAccountFromFile(pf, userName, &cr))
    {
        if (strcmp(userName, u.name) == 0 && cr.accountNbr == r.accountNbr)
        {
            printf("✖ This Account already exists for this user\n\n");
            goto noAccount;
        }
    }
    printf("\nEnter the country:");
    scanf("%s", r.country);
    printf("\nEnter the phone number:");
    scanf("%d", &r.phone);
    printf("\nEnter amount to deposit: $");
    scanf("%lf", &r.amount);
    printf("\nChoose the type of account:\n\t-> saving\n\t-> current\n\t-> fixed01(for 1 year)\n\t-> fixed02(for 2 years)\n\t-> fixed03(for 3 years)\n\n\tEnter your choice:");
    scanf("%s", r.accountType);

    r.userId = u.id;
    strcpy(r.name, u.name);
    saveAccountToFile(pf, r);

    fclose(pf);
    success(u);
}

void checkAllAccounts(struct User u)
{
    char userName[100];
    struct Record r;

    FILE *pf = fopen(RECORDS, "r");

    system("clear");
    printf("\t\t====== All accounts from user, %s =====\n\n", u.name);
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0)
        {
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:%d \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.day,
                   r.deposit.month,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
        }
    }
    fclose(pf);
    success(u);
}

void checkAcc(struct User u)
{
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    struct Record r;
    int found = 0;
    double interest;


    FILE *pf = fopen(RECORDS, "r");
    while (getAccountFromFile(pf, userName, &r))
    {
        if (strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {   
            printf("_____________________\n");
            printf("\nAccount number:%d\nDeposit Date:%d/%d/%d \ncountry:%s \nPhone number:%d \nAmount deposited: $%.2f \nType Of Account:%s\n",
                   r.accountNbr,
                   r.deposit.day,
                   r.deposit.month,
                   r.deposit.year,
                   r.country,
                   r.phone,
                   r.amount,
                   r.accountType);
            found = 1;
            if (strcmp(r.accountType, "savings") == 0) 
            {   
                interest= r.amount * 0.07;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on day %d of every month\n",interest, r.deposit.day);

            }
            else if (strcmp(r.accountType, "fixed01") == 0) 
            {   
                interest= r.amount * 0.04;
                printf("_____________________\n");
                printf("You will get $%.2f as interest on day %d of every month\n",interest, r.deposit.day);

            }
            else if (strcmp(r.accountType, "fixed02") == 0)
            {
                interest= r.amount * 0.05;
                printf("_____________________\n");  
                printf("You will get $%.2f as interest on day %d of every month\n",interest, r.deposit.day);
            }
            else if (strcmp(r.accountType, "fixed03") == 0)
            {
                interest= r.amount * 0.08;
                printf("_____________________\n");  
                printf("You will get $%.2f as interest on day %d of every month\n",interest, r.deposit.day);
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

void makeTransaction(struct User u)
{
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
    while (getAccountFromFile(pf, userName, &r))
    {

    if (strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            do
            {
            printf("_____________________\n");
            printf("\nDeposit: 1 or Withdrawal: 2 ?");
            scanf("%d", &DorW);
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
                    printf("\nWithdrawal denied. Insufficient funds.");
                    printf("Available balance: $%.2f\n", r.amount);
                }
            } 
            }
            while (DorW != 1 && DorW != 2);
            found = 1;
        } else
            {
        saveAccountToFile(temp, r);
            }
        
    } 
    fclose(pf);
    fclose(temp);
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, makeTransaction, u);
    success(u);
}    


void updateAcc(struct User u)
{

    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    struct Record r;
    int found = 0;

    FILE *pf= fopen(RECORDS, "r");
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
                scanf("%d", &found);
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

void removeAcc(struct User u)
{
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
    remove(RECORDS);
    rename("temp.txt", RECORDS);
    if (!found)
        stayOrReturn(0, removeAcc, u);
    success(u);
}    

void transferOwner(struct User u)
{
    int accNbr;
    printf("\nEnter the account number:");
    scanf("%d", &accNbr);

    char userName[100];
    char newUserName[100];
    struct Record r;
    int found = 0;  

    FILE *pf= fopen(RECORDS, "r");
    FILE *temp = fopen("temp.txt", "w");
    while (getAccountFromFile(pf, userName, &r))
    {
        if(strcmp(userName, u.name) == 0 && (r.accountNbr == accNbr))
        {
            printf("_____________________\n");
            printf("\nEnter the new owner name:");
            scanf("%s", newUserName);
            
            strcpy(r.name, newUserName);
            saveAccountToFile(temp, r);
            found = 1;
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
        stayOrReturn(0, transferOwner, u);
    success(u);
}



        









    

    
