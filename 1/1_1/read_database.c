#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_USERS 100
#define LOGIN 6
#define SANCTION "12345"

typedef struct {
    char login[LOGIN + 1];    // 7 байт
    unsigned long pinHash;    // Число (8 байт на 64-битной системе)
    int sanctionLimit;        // 4 байта
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

// Проверка валидности логина
int isValidLogin(const char *login) {
    int len = strlen(login);
    if (len == 0 || len > LOGIN) return -1;
    for (int i = 0; i < len; i++) {
        if (!isalnum(login[i])) return -1;
    }
    return 1;
}

// Инициализация базы данных
void initDatabase(UserDatabase *db, const char *filePath) {
    memset(db, 0, sizeof(UserDatabase));
    for (int i = 0; i < MAX_USERS; i++) {
        db->users[i] = NULL;
    }
    strncpy(db->dbFilePath, filePath, sizeof(db->dbFilePath) - 1);
    db->dbFilePath[sizeof(db->dbFilePath) - 1] = '\0';
}

// Освобождение памяти
void freeDatabase(UserDatabase *db) {
    for (int i = 0; i < db->count; i++) {
        free(db->users[i]);
    }
    db->count = 0;
}

// Функция для вывода содержимого базы данных
void printDatabase(const char *filePath) {
    UserDatabase db;
    initDatabase(&db, filePath);

    FILE* file = fopen(db.dbFilePath, "rb");
    if (!file) {
        printf("Error: Could not open database file '%s'.\n", db.dbFilePath);
        return;
    }

    size_t totalRead = 0;
    while (db.count < MAX_USERS) {
        User* user = malloc(sizeof(User));
        if (!user) {
            printf("Error: Memory allocation failed.\n");
            break;
        }

        size_t bytesRead = fread(user, sizeof(User), 1, file);
        if (bytesRead != 1) {
            free(user);
            if (feof(file)) {
                break; // Конец файла
            }
            printf("Error: Failed to read user data at entry %d.\n", db.count + 1);
            break;
        }

        // Проверка валидности логина
        if (isValidLogin(user->login) == -1) {
            printf("Warning: Invalid login detected at entry %d. Skipping.\n", db.count + 1);
            free(user);
            continue;
        }

        db.users[db.count++] = user;
        totalRead += sizeof(User);
    }
    fclose(file);

    if (db.count == 0) {
        printf("Database is empty or no valid entries found in '%s'.\n", filePath);
    } else {
        printf("\nDatabase contents (%s):\n", filePath);
        for (int i = 0; i < db.count; i++) {
            char pinHashStr[32];
            snprintf(pinHashStr, sizeof(pinHashStr), "%lu", db.users[i]->pinHash); // Преобразуем хеш в строку
            printf("User %d: Login: %s, PIN Hash: %s, Sanction Limit: %d\n",
                   i + 1, db.users[i]->login, pinHashStr, db.users[i]->sanctionLimit);
        }
        printf("Total users read: %d\n", db.count);
    }

    freeDatabase(&db);
}

int main(int argc, char *argv[]) {
    const char *filePath = "users.txt";
    if (argc > 1) {
        filePath = argv[1];
    }

    printDatabase(filePath);
    return 0;
}