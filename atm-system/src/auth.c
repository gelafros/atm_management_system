#include <termios.h>
#include "header.h"

char *USERS = "./data/users.txt";

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

void loginMenu(char a[50], char pass[50])
{
    system("clear");
    printf("\n\n\n\t\t\t\t   Bank Management System\n\t\t\t\t\t User Login:");
    scanf("%s", a);
    while (getchar() != '\n'); // Clear the newline left in the buffer by scanf

    printf("\n\n\n\n\n\t\t\t\tEnter the password to login:");
    getMaskedPassword(pass);
}

const char *getPassword(struct User u)
{
    FILE *fp;
    struct User userChecker;

    if ((fp = fopen("./data/users.txt", "r")) == NULL)
    {
        printf("Error! opening file");
        exit(1);
    }

    while (fscanf(fp, "%s %s", userChecker.name, userChecker.password) != EOF)
    {
        if (strcmp(userChecker.name, u.name) == 0)
        {
            fclose(fp);
            char *buff = userChecker.password;
            return buff;
        }
    }

    fclose(fp);
    return "no user found";
}