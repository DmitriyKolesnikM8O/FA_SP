#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>

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
    char *dbFilePath;
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
    printf("\nДоступные команды:\n");
    printf("  Time - показать текущее время\n");
    printf("  Date - показать текущую дату\n");
    printf("  Howmuch <дата> <флаг> - показать прошедшее время\n");
    printf("    Пример: Howmuch 01.01.2024 -s\n");
    printf("    Флаги: -s (секунды), -m (минуты), -h (часы), -y (годы)\n");
    printf("  Sanctions <username> <число> - установить ограничения\n");
    printf("    Пример: Sanctions user1 10\n");
    printf("  Logout - выйти из системы\n");
    printf("  Help - показать это сообщение\n\n");
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
    // Вычисляем длину числа
    int len = snprintf(NULL, 0, "%ld", pin) + 1;
    char *temp = malloc(len);
    if (!temp) {
        printf("Ошибка: не удалось выделить память.\n");
        return -1;
    }
    
    snprintf(temp, len, "%ld", pin);
    *output = 0;
    int i = 0;
    while (temp[i] != '\0') {
        *output += temp[i] * 31;
        i++;
    }

    free(temp);
    return 0;
}

int setupDatabase(UserDatabase* db, const char* filePath) {
    if (db == NULL) {
        printf("Ошибка: неверный указатель на базу данных.\n");
        return 0;
    }
    if (filePath == NULL) {
        printf("Ошибка: неверный путь к файлу.\n");
        return 0;
    }

    db->count = 0;
    int i;
    for (i = 0; i < MAX_USERS; i++) {
        db->users[i] = NULL;
    }
    
    db->dbFilePath = strdup(filePath);
    if (!db->dbFilePath) {
        printf("Ошибка: не удалось выделить память для пути к файлу.\n");
        return 0;
    }
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
        printf("Текущее время: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else {
        printf("Ошибка: не удалось получить текущее время.\n");
    }

    return 0;
}

int showCurrentDate() {
    time_t t;
    time(&t);
    struct tm *tm = localtime(&t);
    if (tm) {
        printf("Текущая дата: %02d.%02d.%04d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
    } else {
        printf("Ошибка: не удалось получить текущую дату.\n");
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
        printf("Ошибка: не удалось обработать дату.\n");
        return 0;
    }

    time_t now;
    time(&now);
    double diff = difftime(now, past);
    if (diff < 0) {
        printf("Ошибка: указанная дата находится в будущем.\n");
        return 0;
    }

    if (flag[1] == 's') printf("Прошло времени: %.0f секунд\n", diff);
    else if (flag[1] == 'm') printf("Прошло времени: %.0f минут\n", diff / 60);
    else if (flag[1] == 'h') printf("Прошло времени: %.0f часов\n", diff / 3600);
    else if (flag[1] == 'y') printf("Прошло времени: %.2f лет\n", diff / (3600 * 24 * 365.25));

    return 0;
}

int storeUsers(const UserDatabase* db) {
    if (db == NULL) {
        printf("Ошибка: неверный указатель на базу данных.\n");
        return 0;
    }

    FILE* file = fopen(db->dbFilePath, "wb");
    if (!file) {
        printf("Ошибка открытия файла для сохранения данных пользователей");
        return 0;
    }
    int i = 0;
    while (i < db->count) {
        if (db->users[i] == NULL) {
            printf("Ошибка: обнаружена пустая запись пользователя с индексом %d.\n", i);
            fclose(file);
            return 0;
        }
        if (fwrite(db->users[i], sizeof(User), 1, file) != 1) {
            printf("Ошибка: не удалось записать данные пользователя %d.\n", i);
            fclose(file);
            return 0;
        }
        i++;
    }
    fclose(file);
    return 1;
}

int handleTimeElapsed() {
    char *dateStr = NULL;
    char *flag = NULL;
    size_t date_size = 0;
    size_t flag_size = 0;
    
    printf("Input date (dd.mm.yyyy): ");
    if (getline(&dateStr, &date_size, stdin) == -1) {
        return 0;
    }
    if (dateStr[strlen(dateStr) - 1] == '\n') {
        dateStr[strlen(dateStr) - 1] = '\0';
    }

    printf("Select unit (-s, -m, -h, -y): ");
    if (getline(&flag, &flag_size, stdin) == -1) {
        free(dateStr);
        return 0;
    }
    if (flag[strlen(flag) - 1] == '\n') {
        flag[strlen(flag) - 1] = '\0';
    }

    int day, month, year;
    if (!checkTimeElapsedInput(dateStr, &day, &month, &year, flag)) {
        printf("Invalid date or unit (-s, -m, -h, or -y).\n");
        free(dateStr);
        free(flag);
        return 0;
    }

    calculateTimeElapsed(day, month, year, flag);
    
    free(dateStr);
    free(flag);
    return 0;
}

int checkRestrictionInput(const char *targetLogin, const char *limitStr, const char *confirm) {
    if (verifyLogin(targetLogin) == -1) return 0;
    int limit = atoi(limitStr);
    if (limit < 0 || strcmp(confirm, SANCTION) != 0) return 0;
    return 1;
}

int setUserRestriction(UserDatabase* db, const char *targetLogin, int limit) {
    User *targetUser = locateUser(db, targetLogin);
    if (!targetUser) {
        printf("Ошибка: пользователь не найден.\n");
        return 0;
    }
    if (limit < 0) {
        printf("Ошибка: ограничение не может быть отрицательным числом.\n");
        return 0;
    }
    targetUser->sanctionLimit = limit;
    if (!storeUsers(db)) {
        printf("Ошибка: не удалось сохранить изменения ограничений.\n");
        targetUser->sanctionLimit = -1;
        return 0;
    }
    printf("Ограничения успешно установлены!\n");

    return 0;
}

int handleUserRestriction(UserDatabase* db) {
    char *targetLogin = NULL;
    char *limitStr = NULL;
    char *confirm = NULL;
    size_t login_size = 0;
    size_t limit_size = 0;
    size_t confirm_size = 0;
    
    printf("Enter login to restrict: ");
    if (getline(&targetLogin, &login_size, stdin) == -1) {
        return 0;
    }
    if (targetLogin[strlen(targetLogin) - 1] == '\n') {
        targetLogin[strlen(targetLogin) - 1] = '\0';
    }

    printf("Set command limit: ");
    if (getline(&limitStr, &limit_size, stdin) == -1) {
        free(targetLogin);
        return 0;
    }
    if (limitStr[strlen(limitStr) - 1] == '\n') {
        limitStr[strlen(limitStr) - 1] = '\0';
    }

    printf("Enter confirmation code: ");
    if (getline(&confirm, &confirm_size, stdin) == -1) {
        free(targetLogin);
        free(limitStr);
        return 0;
    }
    if (confirm[strlen(confirm) - 1] == '\n') {
        confirm[strlen(confirm) - 1] = '\0';
    }

    if (!checkRestrictionInput(targetLogin, limitStr, confirm)) {
        printf("Invalid input or wrong confirmation code.\n");
        free(targetLogin);
        free(limitStr);
        free(confirm);
        return 0;
    }

    int limit = atoi(limitStr);
    setUserRestriction(db, targetLogin, limit);
    
    free(targetLogin);
    free(limitStr);
    free(confirm);
    return 0;
}

int fetchUsers(UserDatabase* db) {
    FILE* file = fopen(db->dbFilePath, "rb");
    if (!file) return 0;

    while (db->count < MAX_USERS) {
        User* user = malloc(sizeof(User));
        if (!user) {
            printf("Ошибка при аллокации памяти при загрузке бинарных данных.\n");
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
    free(db->dbFilePath);
    return 0;
}

int addUser(UserDatabase* db, const char *login, long int pin) {
    if (db->count >= MAX_USERS) {
        printf("Ошибка: достигнут лимит пользователей.\n");
        return 0;
    }
    if (verifyLogin(login) == -1) {
        printf("Ошибка: логин должен содержать от 1 до %d букв и цифр.\n", LOGIN);
        return 0;
    }
    if (locateUser(db, login)) {
        printf("Ошибка: такой логин уже существует!\n");
        return 0;
    }

    User *newUser = malloc(sizeof(User));
    if (!newUser) {
        printf("Ошибка: не удалось выделить память.\n");
        return 0;
    }
    strcpy(newUser->login, login);
    encryptPin(pin, &newUser->pinHash);
    newUser->sanctionLimit = -1;

    db->users[db->count] = newUser;
    db->count++;
    
    if (!storeUsers(db)) {
        printf("Ошибка: не удалось сохранить нового пользователя в базе данных.\n");
        free(newUser);
        db->users[db->count - 1] = NULL;
        db->count--;
        return 0;
    }
    printf("Пользователь успешно зарегистрирован!\n");
    return 1;
}

int userSession(UserDatabase* db, const User* currentUser) {
    char *input = NULL;
    size_t input_size = 0;
    int sessionCommandCount = 0;

    while (1) {
        printf("\n%s> ", currentUser->login);
        ssize_t read = getline(&input, &input_size, stdin);
        if (read == -1) {
            printf("Сессия завершена.\n");
            break;
        }
        if (input[read - 1] == '\n') {
            input[read - 1] = '\0';
        }

        if (strlen(input) == 0) continue;

        if (currentUser->sanctionLimit >= 0 && sessionCommandCount >= currentUser->sanctionLimit) {
            printf("Достигнут лимит запросов. Доступна только команда Logout.\n");
            if (strcmp(input, "Logout") == 0) {
                printf("Выход из системы.\n");
                free(input);
                return 0;
            }
            continue;
        }

        if (strcmp(input, "Logout") == 0) {
            printf("Выход из системы.\n");
            free(input);
            return 0;
        }
        else if (strcmp(input, "Time") == 0) {
            showCurrentTime();
            sessionCommandCount++;
        }
        else if (strcmp(input, "Date") == 0) {
            showCurrentDate();
            sessionCommandCount++;
        }
        else if (strncmp(input, "Howmuch ", 8) == 0) {
            char *dateStr = NULL;
            char *flag = NULL;
            size_t date_size = 0;
            size_t flag_size = 0;
            
            if (sscanf(input + 8, "%ms %ms", &dateStr, &flag) != 2) {
                printf("Ошибка: неверный формат команды.\n");
                printf("Пример: Howmuch 01.01.2024 -s\n");
                continue;
            }
            
            int day, month, year;
            if (!checkTimeElapsedInput(dateStr, &day, &month, &year, flag)) {
                printf("Ошибка: неверный формат даты или флага.\n");
                printf("Используйте формат: DD.MM.YYYY и флаг -s, -m, -h или -y\n");
                free(dateStr);
                free(flag);
                continue;
            }
            
            calculateTimeElapsed(day, month, year, flag);
            free(dateStr);
            free(flag);
            sessionCommandCount++;
        }
        else if (strncmp(input, "Sanctions ", 10) == 0) {
            char *targetLogin = NULL;
            char *limitStr = NULL;
            size_t login_size = 0;
            size_t limit_size = 0;
            
            if (sscanf(input + 10, "%ms %ms", &targetLogin, &limitStr) != 2) {
                printf("Ошибка: неверный формат команды.\n");
                printf("Пример: Sanctions user1 10\n");
                continue;
            }

            printf("Введите код подтверждения: ");
            char *confirm = NULL;
            size_t confirm_size = 0;
            if (getline(&confirm, &confirm_size, stdin) == -1) {
                free(targetLogin);
                free(limitStr);
                continue;
            }
            if (confirm[strlen(confirm) - 1] == '\n') {
                confirm[strlen(confirm) - 1] = '\0';
            }

            if (!checkRestrictionInput(targetLogin, limitStr, confirm)) {
                printf("Ошибка: неверный ввод или код подтверждения.\n");
                free(targetLogin);
                free(limitStr);
                free(confirm);
                continue;
            }

            int limit = atoi(limitStr);
            setUserRestriction(db, targetLogin, limit);
            free(targetLogin);
            free(limitStr);
            free(confirm);
            sessionCommandCount++;
        }
        else if (strcmp(input, "Help") == 0) {
            displayMenu();
        }
        else {
            printf("Неизвестная команда. Введите Help для списка команд.\n");
        }
    }

    free(input);
    return 0;
}

int loginScreen(UserDatabase* db) {
    fetchUsers(db);

    while (1) {
        printf("\n=== Система авторизации ===\n");
        printf("1. Вход\n");
        printf("2. Регистрация\n");
        printf("3. Выход\n");
        printf("Выберите действие: ");
        
        char *choiceStr = NULL;
        size_t choice_size = 0;
        if (getline(&choiceStr, &choice_size, stdin) == -1) {
            printf("Программа завершена.\n");
            break;
        }
        if (choiceStr[strlen(choiceStr) - 1] == '\n') {
            choiceStr[strlen(choiceStr) - 1] = '\0';
        }
        
        if (strlen(choiceStr) == 0) {
            free(choiceStr);
            continue;
        }

        char *endptr;
        long choice = strtol(choiceStr, &endptr, 10);
        free(choiceStr);
        
        if (*endptr != '\0' || choice < 1 || choice > 3) {
            printf("Ошибка: выберите число от 1 до 3.\n");
            continue;
        }
        if (choice == 3) {
            printf("Завершение работы.\n");
            break;
        }

        char *login = NULL;
        size_t login_size = 0;
        printf("Введите логин (до %d символов, только буквы и цифры): ", LOGIN);
        if (getline(&login, &login_size, stdin) == -1) {
            continue;
        }
        if (login[strlen(login) - 1] == '\n') {
            login[strlen(login) - 1] = '\0';
        }
        
        if (strlen(login) == 0) {
            free(login);
            continue;
        }

        if (verifyLogin(login) == -1) {
            printf("Ошибка: логин должен содержать от 1 до %d букв и цифр.\n", LOGIN);
            free(login);
            continue;
        }

        printf("Введите PIN-код (0-100000): ");
        char *pinInput = NULL;
        size_t pin_size = 0;
        if (getline(&pinInput, &pin_size, stdin) == -1) {
            free(login);
            continue;
        }
        if (pinInput[strlen(pinInput) - 1] == '\n') {
            pinInput[strlen(pinInput) - 1] = '\0';
        }
        
        if (strlen(pinInput) == 0) {
            free(login);
            free(pinInput);
            continue;
        }
        
        if (strlen(pinInput) > 1 && pinInput[0] == '0') {
            printf("Ошибка: PIN-код не может начинаться с нуля.\n");
            free(login);
            free(pinInput);
            continue;
        }
        
        int validPin = 1;
        for (int i = 0; pinInput[i] != '\0'; i++) {
            if (!isdigit(pinInput[i])) {
                validPin = 0;
                break;
            }
        }
        
        if (!validPin) {
            printf("Ошибка: PIN-код должен содержать только цифры.\n");
            free(login);
            free(pinInput);
            continue;
        }
        
        long pin = strtol(pinInput, &endptr, 10);
        free(pinInput);
        
        if (*endptr != '\0' || pin < 0 || pin > 100000) {
            printf("Ошибка: PIN-код должен быть числом от 0 до 100000.\n");
            free(login);
            continue;
        }

        unsigned long inputPinHash;
        encryptPin(pin, &inputPinHash);

        if (choice == 1) {
            User *user = locateUser(db, login);
            if (!user || user->pinHash != inputPinHash) {
                printf("Ошибка: неверный логин или PIN-код!\n");
                free(login);
                continue;
            }
            printf("Добро пожаловать, %s!\n", login);
            userSession(db, user);
        } else {
            if (locateUser(db, login)) {
                printf("Ошибка: такой логин уже существует!\n");
                free(login);
                continue;
            }
            if (addUser(db, login, pin)) {
                printf("Регистрация успешно завершена!\n");
                userSession(db, db->users[db->count - 1]);
            }
        }
        free(login);
    }
    if (!storeUsers(db)) {
        printf("Предупреждение: не удалось сохранить данные пользователей при выходе.\n");
    }

    return 0;
}

int main() {
    UserDatabase db;
    if (!setupDatabase(&db, "users.txt")) {
        printf("Ошибка: не удалось инициализировать базу данных. Выход.\n");
        return 1;
    }
    int running = 1;

    while (running) {
        printf("\n=== Главное меню ===\n");
        printf("1. Войти в оболочку (Вход/Регистрация)\n");
        printf("2. Показать доступные команды\n");
        printf("3. Выход\n");
        printf("Выберите действие: ");

        char *choiceStr = NULL;
        size_t choice_size = 0;
        if (getline(&choiceStr, &choice_size, stdin) == -1) {
            printf("Программа завершена.\n");
            break;
        }
        if (choiceStr[strlen(choiceStr) - 1] == '\n') {
            choiceStr[strlen(choiceStr) - 1] = '\0';
        }
        
        if (strlen(choiceStr) == 0) {
            free(choiceStr);
            continue;
        }

        char *endptr;
        long choice = strtol(choiceStr, &endptr, 10);
        free(choiceStr);
        
        if (*endptr != '\0' || choice < 1 || choice > 3) {
            printf("Ошибка: выберите число от 1 до 3.\n");
            continue;
        }

        switch (choice) {
            case 1:
                loginScreen(&db);
                break;
            case 2:
                printf("\n=== Список доступных команд ===\n");
                displayMenu();
                break;
            case 3:
                printf("Завершение работы программы.\n");
                running = 0;
                break;
        }
    }

    cleanupDatabase(&db);
    return 0;
}