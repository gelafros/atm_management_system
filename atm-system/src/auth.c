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
    scanf("%s", a);
    while (getchar() != '\n'); // Clear the newline left in the buffer by scanf

    printf("\n\n\n\n\n\t\t\t\tEnter the password to login:");
    getMaskedPassword(pass);
}

// getPassword retrieves and decrypts the password for a given user from the users data file.
// It returns a pointer to a static buffer containing the decrypted password, or an error string if the user is not found.
const char *getPassword(struct User u)
{
    FILE *fp;
    struct User userChecker;

    if ((fp = fopen("./data/users.txt", "r")) == NULL)
    {
        printf("Error! opening file");
        exit(1);
    }

    // Fix: fscanf must read the ID first before the name and password!
    while (fscanf(fp, "%d %s %s", &userChecker.id, userChecker.name, userChecker.password) != EOF)
    {
        if (strcmp(userChecker.name, u.name) == 0)
        {
            fclose(fp);
            
            // Use a static buffer to safely return the decrypted password
            static char decrypted[50];
            strcpy(decrypted, userChecker.password);
            
            // Decrypt the password (shift back by -3)
            int i = 0;
            while (decrypted[i] != '\0')
            {
                decrypted[i] = decrypted[i] - 3;
                i++;
            }
            
            return decrypted;
        }
    }

    fclose(fp);
    return "no user found";
}