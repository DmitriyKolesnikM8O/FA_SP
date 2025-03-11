#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "utility.h"


void hashPin(long int pin, char *output) {
    char pinStr[7];
    snprintf(pinStr, sizeof(pinStr), "%ld", pin);
    unsigned long hash = 0;
    for (int i = 0; pinStr[i]; i++) {
        hash += pinStr[i] * 31;
    }
    snprintf(output, 32, "%lu", hash);
}

void printMessage(const char *message) {
    printf("%s\n", message);
}

void initDatabase(UserDatabase* db, const char* filePath) {
    memset(db, 0, sizeof(UserDatabase));
    strncpy(db->dbFilePath, filePath, sizeof(db->dbFilePath) - 1);
    db->dbFilePath[sizeof(db->dbFilePath) - 1] = '\0';
}

void freeDatabase(UserDatabase* db) {
    for (int i = 0; i < db->count; i++) {
        free(db->users[i]);
    }
    db->count = 0;
}

int loadUsers(UserDatabase* db) {
    FILE* file = fopen(db->dbFilePath, "rb");
    if (!file) return 0;

    while (db->count < MAX_USERS) {
        User* user = malloc(sizeof(User));
        if (!user) {
            printMessage(errorMessages[1].message);
            break;
        }
        if (fread(user, sizeof(User), 1, file) != 1) {
            free(user);
            break;
        }
        db->users[db->count++] = user;
    }
    fclose(file);
    return 1;
}

int saveUsers(const UserDatabase* db) {
    FILE* file = fopen(db->dbFilePath, "wb");
    if (!file) {
        printMessage("Error: Cannot save users to file.");
        return 0;
    }
    for (int i = 0; i < db->count; i++) {
        fwrite(db->users[i], sizeof(User), 1, file);
    }
    fclose(file);
    return 1;
}

User* findUser(const UserDatabase* db, const char *login) {
    for (int i = 0; i < db->count; i++) {
        if (strcmp(db->users[i]->login, login) == 0) {
            return db->users[i];
        }
    }
    return NULL;
}

int isValidLogin(const char *login) {
    int len = strlen(login);
    if (len == 0 || len > MAX_LOGIN_LENGTH) return -1;
    for (int i = 0; i < len; i++) {
        if (!isalnum(login[i])) return -1;
    }
    return 1;
}

int registerUser(UserDatabase* db, const char *login, long int pin) {
    if (db->count >= MAX_USERS) {
        printMessage(errorMessages[0].message);
        return 0;
    }
    if (isValidLogin(login) == -1) {
        printMessage("Error: Login must be 1-6 alphanumeric characters.");
        return 0;
    }
    if (findUser(db, login)) {
        printMessage(errorMessages[14].message);
        return 0;
    }

    User *newUser = malloc(sizeof(User));
    if (!newUser) {
        printMessage(errorMessages[1].message);
        return 0;
    }
    strncpy(newUser->login, login, MAX_LOGIN_LENGTH);
    newUser->login[MAX_LOGIN_LENGTH] = '\0';
    hashPin(pin, newUser->pinHash);
    newUser->sanctionLimit = 0;

    db->users[db->count++] = newUser;
    saveUsers(db);
    printMessage(successMessages[0].message);
    return 1;
}

void showCommands() {
    printf("\nAvailable commands:\n");
    for (size_t i = 0; i < sizeof(commandDescriptions) / sizeof(CommandDescription); i++) {
        printf("  %ld. %s - %s\n", i + 1, commandDescriptions[i].command, commandDescriptions[i].description);
    }
}

void processTime() {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        printf("Current time: %02d:%02d:%02d\n", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    }
}

void processDate() {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        printf("Current date: %02d.%02d.%04d\n", tm_info->tm_mday, tm_info->tm_mon + 1, tm_info->tm_year + 1900);
    }
}

void processHowMuch() {
    char dateStr[16], flag[4];
    printf("%s", userPrompts[0].prompt);
    if (!fgets(dateStr, sizeof(dateStr), stdin)) return;
    dateStr[strcspn(dateStr, "\n")] = '\0';

    int day, month, year;
    if (sscanf(dateStr, "%d.%d.%d", &day, &month, &year) != 3 ||
        day < 1 || day > 31 || month < 1 || month > 12 || year < 1900) {
        printMessage(userPrompts[0].error);
        return;
    }

    struct tm input_tm = {0};
    input_tm.tm_mday = day;
    input_tm.tm_mon = month - 1;
    input_tm.tm_year = year - 1900;
    input_tm.tm_isdst = -1;

    time_t input_time = mktime(&input_tm);
    if (input_time == -1) {
        printMessage(errorMessages[5].message);
        return;
    }

    printf("%s", userPrompts[1].prompt);
    if (!fgets(flag, sizeof(flag), stdin)) return;
    flag[strcspn(flag, "\n")] = '\0';

    time_t now = time(NULL);
    double diff = difftime(now, input_time);
    if (diff < 0) {
        printMessage(errorMessages[6].message);
        return;
    }

    if (strcmp(flag, "-s") == 0) printf("Elapsed: %.0f seconds\n", diff);
    else if (strcmp(flag, "-m") == 0) printf("Elapsed: %.0f minutes\n", diff / 60);
    else if (strcmp(flag, "-h") == 0) printf("Elapsed: %.0f hours\n", diff / 3600);
    else if (strcmp(flag, "-y") == 0) printf("Elapsed: %.2f years\n", diff / (3600 * 24 * 365.25));
    else printMessage(errorMessages[7].message);
}

void processSanctions(UserDatabase* db) {
    char targetLogin[MAX_LOGIN_LENGTH + 1];
    printf("%s", userPrompts[2].prompt);
    if (!fgets(targetLogin, sizeof(targetLogin), stdin)) return;
    targetLogin[strcspn(targetLogin, "\n")] = '\0';

    User *targetUser = findUser(db, targetLogin);
    if (!targetUser) {
        printMessage(errorMessages[8].message);
        return;
    }

    char limitStr[10];
    printf("%s", userPrompts[3].prompt);
    if (!fgets(limitStr, sizeof(limitStr), stdin)) return;
    limitStr[strcspn(limitStr, "\n")] = '\0';

    char *endptr;
    long limit = strtol(limitStr, &endptr, 10);
    if (*endptr != '\0' || limit <= 0) {
        printMessage(errorMessages[9].message);
        return;
    }

    printf("%s", userPrompts[4].prompt);
    char confirm[10];
    if (!fgets(confirm, sizeof(confirm), stdin)) return;
    confirm[strcspn(confirm, "\n")] = '\0';
    if (strcmp(confirm, SANCTION_CODE) != 0) {
        printMessage(errorMessages[10].message);
        return;
    }

    targetUser->sanctionLimit = (int)limit;
    saveUsers(db);
    printMessage(successMessages[1].message);
}

void session(UserDatabase* db, const User* currentUser) {
    char input[10];
    int sessionCommandCount = 0;

    while (1) {
        showCommands();
        printf("\nChoose command number (1-5): ");
        if (!fgets(input, sizeof(input), stdin)) {
            printMessage("Program terminated with EOF.");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "5") == 0) {
            printMessage(successMessages[2].message);
            return;
        }

        if (currentUser->sanctionLimit > 0 && sessionCommandCount >= currentUser->sanctionLimit) {
            printMessage(successMessages[3].message);
            continue;
        }

        if (strcmp(input, "1") == 0) {
            processTime();
            sessionCommandCount++;
        }
        else if (strcmp(input, "2") == 0) {
            processDate();
            sessionCommandCount++;
        }
        else if (strcmp(input, "3") == 0) {
            processHowMuch();
            sessionCommandCount++;
        }
        else if (strcmp(input, "4") == 0) {
            processSanctions(db);
            sessionCommandCount++;
        }
        else {
            printMessage(errorMessages[11].message);
        }
    }
}

void authMenu(UserDatabase* db) {
    loadUsers(db);

    while (1) {
        printf("\n1. Login\n2. Register\n3. Exit\nChoose: ");
        char choiceStr[10];
        if (!fgets(choiceStr, sizeof(choiceStr), stdin)) {
            printMessage("Program terminated with EOF.");
            break;
        }
        choiceStr[strcspn(choiceStr, "\n")] = '\0';

        char *endptr;
        long choice = strtol(choiceStr, &endptr, 10);
        if (*endptr != '\0' || choice < 1 || choice > 3) {
            printMessage(errorMessages[12].message);
            continue;
        }
        if (choice == 3) {
            printMessage(successMessages[4].message);
            break;
        }

        char login[MAX_LOGIN_LENGTH + 2];
        printf("%s", userPrompts[5].prompt);
        if (!fgets(login, sizeof(login), stdin)) continue;
        login[strcspn(login, "\n")] = '\0';

        if (isValidLogin(login) == -1) {
            printMessage("Error: Login must be 1-6 alphanumeric characters.");
            continue;
        }

        printf("%s", userPrompts[6].prompt);
        char pinStr[10];
        if (!fgets(pinStr, sizeof(pinStr), stdin)) continue;
        pinStr[strcspn(pinStr, "\n")] = '\0';

        char *pinEndPtr;
        long pin = strtol(pinStr, &pinEndPtr, 10);
        if (*pinEndPtr != '\0' || pin < 0 || pin > 100000) {
            printMessage(errorMessages[13].message);
            continue;
        }

        char inputPinHash[32];
        hashPin(pin, inputPinHash);

        if (choice == 1) {
            User *user = findUser(db, login);
            if (!user || strcmp(user->pinHash, inputPinHash) != 0) {
                printMessage(errorMessages[15].message);
                continue;
            }
            printMessage(successMessages[5].message);
            session(db, user);
        } else {
            if (findUser(db, login)) {
                printMessage(errorMessages[14].message);
                continue;
            }
            if (registerUser(db, login, pin)) {
                session(db, db->users[db->count - 1]);
            }
        }
    }
    saveUsers(db);
}

int main() {
    UserDatabase db;
    initDatabase(&db, "users.txt");
    authMenu(&db);
    freeDatabase(&db);
    return 0;
}