# ATM Management System

## Description
This is a command-line ATM management system built in C. It allows users to register, log in, manage bank accounts, make transactions, and transfer ownership securely using file-based storage.

## Features
- User Authentication (Login/Register)
- Create new bank accounts (Savings, Current, Fixed)
- Update existing account details (Phone number, Country)
- Check account details and calculate monthly interest
- Make transactions (Deposit, Withdraw) with validation
- Remove existing accounts
- Transfer account ownership to other registered users

## How to Compile and Run
To compile the project, run the following command in your terminal:

```bash
gcc src/main.c src/system.c src/auth.c -o atm
./atm
```