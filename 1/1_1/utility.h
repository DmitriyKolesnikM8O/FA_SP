#ifndef UTILITY_H
#define UTILITY_H

#define MAX_USERS 100
#define MAX_LOGIN_LENGTH 6
#define SANCTION_CODE "12345"

typedef struct {
    char login[MAX_LOGIN_LENGTH + 1];
    char pinHash[32];
    int sanctionLimit;
} User;

typedef struct {
    User *users[MAX_USERS];
    int count;
    char dbFilePath[256];
} UserDatabase;

typedef struct {
    const char *message;
} ErrorMessage;

typedef struct {
    const char *message;
} SuccessMessage;

typedef struct {
    const char *command;
    const char *description;
} CommandDescription;

typedef struct {
    const char *prompt;
    const char *error;
} UserPrompt;

ErrorMessage errorMessages[] = {
    {"Maximum number of users reached."},
    {"Memory allocation error."},
    {"Error: Invalid day."},
    {"Error: Invalid month."},
    {"Error: Invalid year."},
    {"Error processing date."},
    {"Error: The specified date is in the future."},
    {"Error: Invalid flag (-s, -m, -h, or -y)."},
    {"Error: User not found."},
    {"Error: Invalid limit value."},
    {"Error: Incorrect confirmation code."},
    {"Error: Unknown command."},
    {"Input error. Enter a number from 1 to 3."},
    {"Error: Invalid PIN."},
    {"Login already taken!"},
    {"Incorrect login or PIN!"}
};

SuccessMessage successMessages[] = {
    {"Registration successful!"},
    {"Sanctions applied successfully!"},
    {"Logging out."},
    {"Command limit exceeded. Only Logout is available."},
    {"Exiting the program."},
    {"Welcome!"}
};

CommandDescription commandDescriptions[] = {
    {"Time", "show current time"},
    {"Date", "show current date"},
    {"HowMuch", "calculate time elapsed since a date"},
    {"Sanctions", "impose sanctions on a user"},
    {"Logout", "log out"}
};

UserPrompt userPrompts[] = {
    {"Enter date (dd.mm.yyyy): ", "Error: Invalid date format."},
    {"Choose flag (-s, -m, -h, -y): ", "Error: Invalid flag."},
    {"Enter the login of the user to impose sanctions on: ", "Error: User not found."},
    {"Enter command limit: ", "Error: Invalid limit value."},
    {"Confirm sanctions by entering the code: ", "Error: Incorrect confirmation code."},
    {"Login (up to 6 characters): ", "Error: Invalid login."},
    {"PIN (0-100000): ", "Error: Invalid PIN."}
};

#endif