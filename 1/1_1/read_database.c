#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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


void printDatabase(const char *filePath) {
    UserDatabase db;

    memset(&db, 0, sizeof(UserDatabase));
    strncpy(db.dbFilePath, filePath, sizeof(db.dbFilePath) - 1);
    db.dbFilePath[sizeof(db.dbFilePath) - 1] = '\0';


    FILE* file = fopen(db.dbFilePath, "rb");
    if (!file) {
        printf("Error: Could not open database file '%s'.\n", db.dbFilePath);
        return;
    }

    while (db.count < MAX_USERS) {
        User* user = malloc(sizeof(User));
        if (!user) {
            printf("Error: Memory allocation failed.\n");
            break;
        }
        if (fread(user, sizeof(User), 1, file) != 1) {
            free(user);
            break;
        }
        db.users[db.count++] = user;
    }
    fclose(file);


    if (db.count == 0) {
        printf("Database is empty.\n");
    } else {
        printf("\nDatabase contents (%s):\n", filePath);
        for (int i = 0; i < db.count; i++) {
            printf("User %d: Login: %s, PIN Hash: %s, Sanction Limit: %d\n",
                   i + 1, db.users[i]->login, db.users[i]->pinHash, db.users[i]->sanctionLimit);
        }
    }

    
    for (int i = 0; i < db.count; i++) {
        free(db.users[i]);
    }
    db.count = 0;
}

int main(int argc, char *argv[]) {
    const char *filePath = "users.txt";
    if (argc > 1) {
        filePath = argv[1]; 
    }

    printDatabase(filePath);
    return 0;
}