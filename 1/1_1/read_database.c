#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utility.h"


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