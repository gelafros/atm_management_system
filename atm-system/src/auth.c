#include <termios.h>
#include "header.h"

char *USERS = "./data/users.txt";

// FIPS 180-4 SHA-256 constants
static const uint32_t K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static uint32_t rotr32(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32U - n));
}

// hashPassword computes a salted SHA-256 hex digest ("atm-v1:" + username + ":" + password)
// so user passwords are stored one-way in SQLite without external library dependencies.
void hashPassword(const char *username, const char *password, char out_hex[65])
{
    uint8_t msg[256];
    memset(msg, 0, sizeof(msg));

    int len = snprintf((char *)msg, 180, "atm-v1:%s:%s", username, password);
    if (len < 0)
        len = 0;
    if (len > 180)
        len = 180;

    uint64_t bit_len = (uint64_t)len * 8ULL;
    size_t pad_len = (size_t)len;
    msg[pad_len++] = 0x80U;
    while ((pad_len % 64U) != 56U)
    {
        msg[pad_len++] = 0x00U;
    }
    for (int i = 7; i >= 0; i--)
    {
        msg[pad_len++] = (uint8_t)((bit_len >> (i * 8)) & 0xFFU);
    }

    uint32_t H[8] = {
        0x6a09e667UL, 0xbb67ae85UL, 0x3c6ef372UL, 0xa54ff53aUL,
        0x510e527fUL, 0x9b05688cUL, 0x1f83d9abUL, 0x5be0cd19UL
    };

    for (size_t chunk = 0; chunk < pad_len; chunk += 64U)
    {
        uint32_t W[64];
        for (int t = 0; t < 16; t++)
        {
            size_t idx = chunk + (size_t)(t * 4);
            W[t] = ((uint32_t)msg[idx] << 24) |
                   ((uint32_t)msg[idx + 1] << 16) |
                   ((uint32_t)msg[idx + 2] << 8) |
                   ((uint32_t)msg[idx + 3]);
        }
        for (int t = 16; t < 64; t++)
        {
            uint32_t s0 = rotr32(W[t - 15], 7) ^ rotr32(W[t - 15], 18) ^ (W[t - 15] >> 3);
            uint32_t s1 = rotr32(W[t - 2], 17) ^ rotr32(W[t - 2], 19) ^ (W[t - 2] >> 10);
            W[t] = W[t - 16] + s0 + W[t - 7] + s1;
        }

        uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
        uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

        for (int t = 0; t < 64; t++)
        {
            uint32_t S1 = rotr32(e, 6) ^ rotr32(e, 11) ^ rotr32(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = h + S1 + ch + K256[t] + W[t];
            uint32_t S0 = rotr32(a, 2) ^ rotr32(a, 13) ^ rotr32(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        H[0] += a; H[1] += b; H[2] += c; H[3] += d;
        H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }

    for (int i = 0; i < 8; i++)
    {
        snprintf(out_hex + (i * 8), 9, "%08x", H[i]);
    }
    out_hex[64] = '\0';
}

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

// getPassword queries the SQLite database for the stored salted SHA-256 password hash
// of the user whose name matches u.name.
// Returns NULL if the username does not exist in the database.
const char *getPassword(struct User u, sqlite3 *db)
{
    sqlite3_stmt *stmt;
    static char stored_hash[65];

    sqlite3_prepare_v2(db,
        "SELECT password FROM users WHERE name = ?;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, u.name, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *pass = (const char *)sqlite3_column_text(stmt, 0);
        strncpy(stored_hash, pass, sizeof(stored_hash) - 1);
        stored_hash[sizeof(stored_hash) - 1] = '\0';
        sqlite3_finalize(stmt);
        return stored_hash;
    }

    sqlite3_finalize(stmt);
    return NULL;
}
