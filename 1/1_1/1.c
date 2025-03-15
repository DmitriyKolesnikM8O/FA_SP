#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#define MAX_USERS 1000
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

int checkLeapYear(int year) {
    if (year % 400 == 0) return 1;
    if (year % 100 == 0) return 0;
    if (year % 4 == 0) return 1;
    return 0;
}

int getMonthDays(int month, int year) {
    if (month == 2) {
        if (checkLeapYear(year)) return 29;
        return 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

int displayMenu() {
    printf("\nCommand list:\n");
    printf("  1. Time - display current time\n");
    printf("  2. Date - display current date\n");
    printf("  3. HowMuch - calculate elapsed time since a date\n");
    printf("  4. Sanctions - set restrictions for a user\n");
    printf("  5. Logout - exit to login menu\n");

    return 0;
}

int flushBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);

    return 0;
}

int verifyLogin(const char *login) {
    int i = 0;
    while (login[i] != '\0') {
        if (!isalnum(login[i])) return -1;
        i++;
    }
    if (i == 0 || i > LOGIN) return -1;
    return 1;
}

int encryptPin(long int pin, unsigned long *output) {
    char temp[7];
    sprintf(temp, "%ld", pin);
    *output = 0;
    int i = 0;
    while (temp[i] != '\0') {
        *output += temp[i] * 31;
        i++;
    }

    return 0;
}

int setupDatabase(UserDatabase* db, const char* filePath) {
    if (db == NULL) {
        printf("Error: Invalid database pointer provided.\n");
        return 0;
    }
    if (filePath == NULL) {
        printf("Error: Invalid file path provided.\n");
        return 0;
    }
    if (strlen(filePath) >= 256) {
        printf("Error: File path exceeds maximum length of 255 characters.\n");
        return 0;
    }

    db->count = 0;
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        db->users[i] = NULL;
    }
    strcpy(db->dbFilePath, filePath);
    return 1;
}

User* locateUser(const UserDatabase* db, const char *login) {
    int i = 0;
    while (i < db->count) {
        if (strcmp(db->users[i]->login, login) == 0) return db->users[i];
        i++;
    }
    return NULL;
}

int showCurrentTime() {
    time_t t;
    time(&t);
    struct tm *tm = localtime(&t);
    if (tm) {
        printf("Time now: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else {
        printf("Failed to get current time.\n");
    }

    return 0;
}

int showCurrentDate() {
    time_t t;
    time(&t);
    struct tm *tm = localtime(&t);
    if (tm) {
        printf("Date today: %02d.%02d.%04d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    } else {
        printf("Failed to get current date.\n");
    }

    return 0;
}


int checkTimeElapsedInput(const char *dateStr, int *day, int *month, int *year, const char *flag) {
    if (flag[0] != '-' || 
    (flag[1] != 's' && flag[1] != 'm' && flag[1] != 'h' && flag[1] != 'y') || 
    flag[2] != '\0') return 0;
    
    int d = 0, m = 0, y = 0;
    int i = 0, part = 0;
    while (dateStr[i] != '\0') {
        if (dateStr[i] == '.') {
            part++;
            i++;
            continue;
        }
        if (part == 0) d = d * 10 + (dateStr[i] - '0');
        else if (part == 1) m = m * 10 + (dateStr[i] - '0');
        else if (part == 2) y = y * 10 + (dateStr[i] - '0');
        i++;
    }

    if (m < 1 || m > 12 || y < 1900) return 0;
    if (d < 1 || d > getMonthDays(m, y)) return 0;

    *day = d;
    *month = m;
    *year = y;
    
    return 1;
}

int calculateTimeElapsed(int day, int month, int year, const char *flag) {
    struct tm input = {0};
    input.tm_mday = day;
    input.tm_mon = month - 1;
    input.tm_year = year - 1900;

    time_t past = mktime(&input);
    if (past == -1) {
        printf("Failed to process the date.\n");
        return 0 ;
    }

    time_t now;
    time(&now);
    double diff = difftime(now, past);
    if (diff < 0) {
        printf("Error: Date is in the future.\n");
        return 0 ;
    }

    if (flag[1] == 's') printf("Time passed: %.0f seconds\n", diff);
    else if (flag[1] == 'm') printf("Time passed: %.0f minutes\n", diff / 60);
    else if (flag[1] == 'h') printf("Time passed: %.0f hours\n", diff / 3600);
    else if (flag[1] == 'y') printf("Time passed: %.2f years\n", diff / (3600 * 24 * 365.25));

    return 0;
}

int storeUsers(const UserDatabase* db) {
    if (db == NULL) {
        printf("Error: Invalid database pointer provided.\n");
        return 0;
    }

    FILE* file = fopen(db->dbFilePath, "wb");
    if (!file) {
        perror("Error opening file for saving user data");
        return 0;
    }
    int i = 0;
    while (i < db->count) {
        if (db->users[i] == NULL) {
            printf("Error: Null user entry found at index %d.\n", i);
            fclose(file);
            return 0;
        }
        if (fwrite(db->users[i], sizeof(User), 1, file) != 1) {
            printf("Error: Failed to write user data for user %d.\n", i);
            fclose(file);
            return 0;
        }
        i++;
    }
    fclose(file);
    return 1;
}

int handleTimeElapsed() {
    char dateStr[32], flag[10];
    printf("Input date (dd.mm.yyyy): ");
    if (!fgets(dateStr, sizeof(dateStr), stdin)) return 0;
    dateStr[strcspn(dateStr, "\n")] = '\0';

    printf("Select unit (-s, -m, -h, -y): ");
    if (!fgets(flag, sizeof(flag), stdin)) return 0;
    flag[strcspn(flag, "\n")] = '\0';

    int day, month, year;
    if (!checkTimeElapsedInput(dateStr, &day, &month, &year, flag)) {
        printf("Invalid date or unit (-s, -m, -h, or -y).\n");
        return 0;
    }

    calculateTimeElapsed(day, month, year, flag);

    return 0;
}

int checkRestrictionInput(const char *targetLogin, const char *limitStr, const char *confirm) {
    if (verifyLogin(targetLogin) == -1) return 0;
    int limit = atoi(limitStr);
    if (limit <= 0 || strcmp(confirm, SANCTION) != 0) return 0;
    return 1;
}

int setUserRestriction(UserDatabase* db, const char *targetLogin, int limit) {
    User *targetUser = locateUser(db, targetLogin);
    if (!targetUser) {
        printf("User not found.\n");
        return 0;
    }
    targetUser->sanctionLimit = limit;
    if (!storeUsers(db)) {
        printf("Error: Failed to save restriction changes.\n");
        targetUser->sanctionLimit = 0; 
        return 0;
    }
    printf("Restrictions set successfully!\n");

    return 0;
}



int handleUserRestriction(UserDatabase* db) {
    char targetLogin[LOGIN + 2], limitStr[10], confirm[10];
    
    printf("Enter login to restrict: ");
    if (!fgets(targetLogin, sizeof(targetLogin), stdin)) return 0;
    targetLogin[strcspn(targetLogin, "\n")] = '\0';

    printf("Set command limit: ");
    if (!fgets(limitStr, sizeof(limitStr), stdin)) return 0;
    limitStr[strcspn(limitStr, "\n")] = '\0';


    printf("Enter confirmation code: ");
    if (!fgets(confirm, sizeof(confirm), stdin)) return 0;
    confirm[strcspn(confirm, "\n")] = '\0';

    if (!checkRestrictionInput(targetLogin, limitStr, confirm)) {
        printf("Invalid input or wrong confirmation code.\n");
        return 0;
    }

    int limit = atoi(limitStr);
    setUserRestriction(db, targetLogin, limit);

    return 0;
}

int fetchUsers(UserDatabase* db) {
    FILE* file = fopen(db->dbFilePath, "rb");
    if (!file) return 0;

    while (db->count < MAX_USERS) {
        User* user = malloc(sizeof(User));
        if (!user) {
            printf("Failed to allocate memory.\n");
            break;
        }
        if (fread(user, sizeof(User), 1, file) != 1) {
            free(user);
            break;
        }
        if (verifyLogin(user->login) == -1) {
            free(user);
            break;
        }
        db->users[db->count] = user;
        db->count++;
    }
    fclose(file);
    return 1;
}

int cleanupDatabase(UserDatabase* db) {
    int i = 0;
    while (i < db->count) {
        free(db->users[i]);
        i++;
    }
    db->count = 0;

    return 0;
}

int addUser(UserDatabase* db, const char *login, long int pin) {
    if (db->count >= MAX_USERS) {
        printf("User limit reached.\n");
        return 0;
    }
    if (verifyLogin(login) == -1) {
        printf("Error: Login must be 1-6 alphanumeric characters.\n");
        return 0;
    }
    if (locateUser(db, login)) {
        printf("This login is already in use!\n");
        return 0;
    }

    User *newUser = malloc(sizeof(User));
    if (!newUser) {
        printf("Failed to allocate memory.\n");
        return 0;
    }
    strcpy(newUser->login, login);
    encryptPin(pin, &newUser->pinHash);
    newUser->sanctionLimit = 0;

    db->users[db->count] = newUser;
    db->count++;
    
    if (!storeUsers(db)) {
        printf("Error: Failed to save new user to database.\n");
        free(newUser);
        db->users[db->count - 1] = NULL;
        db->count--;
        return 0;
    }
    printf("User registered successfully!\n");
    return 1;
}

int userSession(UserDatabase* db, const User* currentUser) {
    char input[10];
    int sessionCommandCount = 0;

    while (1) {
        displayMenu();
        printf("\nSelect command (1-5): ");
        if (!fgets(input, sizeof(input), stdin)) {
            printf("Session ended with EOF.\n");
            break;
        }
        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "5") == 0) {
            printf("Signing out.\n");
            return 0;
        }

        if (currentUser->sanctionLimit > 0 && sessionCommandCount >= currentUser->sanctionLimit) {
            printf("Limit reached. Only SignOut is available.\n");
            continue;
        }

        if (strcmp(input, "1") == 0) {
            showCurrentTime();
            sessionCommandCount++;
        }
        else if (strcmp(input, "2") == 0) {
            showCurrentDate();
            sessionCommandCount++;
        }
        else if (strcmp(input, "3") == 0) {
            handleTimeElapsed();
            sessionCommandCount++;
        }
        else if (strcmp(input, "4") == 0) {
            handleUserRestriction(db);
            sessionCommandCount++;
        }
        else {
            printf("Unknown command entered.\n");
        }
    }

    return 0;
}

int loginScreen(UserDatabase* db) {
    fetchUsers(db);

    while (1) {
        printf("\n1. SignIn\n2. SignUp\n3. Quit\nSelect: ");
        char choiceStr[10];
        if (!fgets(choiceStr, sizeof(choiceStr), stdin)) {
            printf("Terminated with EOF.\n");
            break;
        }
        choiceStr[strcspn(choiceStr, "\n")] = '\0';

        char *endptr;
        long choice = strtol(choiceStr, &endptr, 10);
        if (*endptr != '\0' || choice < 1 || choice > 3) {
            printf("Invalid selection. Use 1-3.\n");
            continue;
        }
        if (choice == 3) {
            printf("Shutting down.\n");
            break;
        }

        char login[LOGIN + 2];
        printf("Login (max 6 chars): ");
        if (!fgets(login, sizeof(login), stdin)) continue;
        login[strcspn(login, "\n")] = '\0';

        if (verifyLogin(login) == -1) {
            printf("Error: Login must be 1-6 alphanumeric chars.\n");
            continue;
        }

        printf("PIN (0-100000): ");
        char pinStr[10];
        if (!fgets(pinStr, sizeof(pinStr), stdin)) continue;
        pinStr[strcspn(pinStr, "\n")] = '\0';

        char *pinEndPtr;
        long pin = strtol(pinStr, &pinEndPtr, 10);
        if (*pinEndPtr != '\0' || pin < 0 || pin > 100000) {
            printf("Error: PIN must be between 0 and 100000.\n");
            continue;
        }

        unsigned long inputPinHash;
        encryptPin(pin, &inputPinHash);

        if (choice == 1) {
            User *user = locateUser(db, login);
            if (!user || user->pinHash != inputPinHash) {
                printf("Wrong login or PIN!\n");
                continue;
            }
            printf("Hello, welcome back!\n");
            userSession(db, user);
        } else {
            if (locateUser(db, login)) {
                printf("Login already exists!\n");
                continue;
            }
            if (addUser(db, login, pin)) {
                userSession(db, db->users[db->count - 1]);
            }
        }
    }
    if (!storeUsers(db)) {
        printf("Warning: Failed to save user data on exit.\n");
    }

    return 0;
}

int main() {
    UserDatabase db;
    if (!setupDatabase(&db, "users.txt")) {
        printf("Failed to initialize database. Exiting.\n");
        return 1;
    }
    int running = 1;

    while (running) {
        printf("\nMain Menu:\n");
        printf("  1. Enter Shell (Login/Register)\n");
        printf("  2. Show Available Commands\n");
        printf("  3. Exit\n");
        printf("Choose an option: ");

        char choiceStr[10];
        if (!fgets(choiceStr, sizeof(choiceStr), stdin)) {
            printf("Program terminated with EOF.\n");
            break;
        }
        choiceStr[strcspn(choiceStr, "\n")] = '\0';

        char *endptr;
        long choice = strtol(choiceStr, &endptr, 10);
        if (*endptr != '\0' || choice < 1 || choice > 3) {
            printf("Please select a valid option (1-3).\n");
            continue;
        }

        switch (choice) {
            case 1:
                loginScreen(&db);
                break;
            case 2:
                displayMenu();
                break;
            case 3:
                printf("Exiting program.\n");
                running = 0;
                break;
        }
    }

    cleanupDatabase(&db);
    return 0;
}