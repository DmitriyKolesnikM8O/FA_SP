#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_USERS 100
#define LOGIN 6
#define SANCTION "12345"

typedef struct {
    char login[LOGIN + 1];    
    unsigned long pinHash;    
    int sanctionLimit;        
} User;

typedef struct {
    User *users[MAX_USERS];
    int count;
    char dbFilePath[256];
} UserDatabase;

int isValidLogin(const char *login) {
    int len = strlen(login);
    if (len == 0 || len > LOGIN) return -1;
    for (int i = 0; i < len; i++) {
        if (!isalnum(login[i])) return -1;
    }
    return 1;
}

void initDatabase(UserDatabase *db, const char *filePath) {
    memset(db, 0, sizeof(UserDatabase));
    for (int i = 0; i < MAX_USERS; i++) {
        db->users[i] = NULL;
    }
    strncpy(db->dbFilePath, filePath, sizeof(db->dbFilePath) - 1);
    db->dbFilePath[sizeof(db->dbFilePath) - 1] = '\0';
}

void freeDatabase(UserDatabase *db) {
    for (int i = 0; i < db->count; i++) {
        free(db->users[i]);
    }
    db->count = 0;
}

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