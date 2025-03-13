#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

#define PATH_MAX 4096

typedef enum {
    SUCCESS,
    INPUT_FILE_ERROR,
    MEMORY_ALLOCATED_ERROR,
    INCORRECT_ARG_FUNCTION
} type_error;

typedef struct {
    type_error type;
    const char *func;
    const char *msg;
} error_msg;

int print_error(error_msg error) {
    if (error.type) {
        fprintf(stderr, "Error - %s: %s\n", error.func, error.msg);
        return error.type;
    }
    return 0;
}

void format_time(time_t mtime, char *time_str) {
    struct tm *tm_info = localtime(&mtime);
    time_t now;
    time(&now);
    double diff = difftime(now, mtime);

    if (diff > 6 * 30 * 24 * 60 * 60) {
        strftime(time_str, 20, "%b %d  %Y", tm_info);
    } else {
        strftime(time_str, 20, "%b %d %H:%M", tm_info);
    }
}

error_msg access_rights(char *res, struct stat *file_info) {
    if (!res || !file_info) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "null pointer received"};
    }

    
    if (S_ISDIR(file_info->st_mode)) res[0] = 'd';
    else if (S_ISLNK(file_info->st_mode)) res[0] = 'l';
    else if (S_ISFIFO(file_info->st_mode)) res[0] = 'p';
    else if (S_ISCHR(file_info->st_mode)) res[0] = 'c';
    else if (S_ISBLK(file_info->st_mode)) res[0] = 'b';
    else if (S_ISSOCK(file_info->st_mode)) res[0] = 's';
    else res[0] = '-';

    
    res[1] = (file_info->st_mode & S_IRUSR) ? 'r' : '-';
    res[2] = (file_info->st_mode & S_IWUSR) ? 'w' : '-';
    res[3] = (file_info->st_mode & S_IXUSR) ? 'x' : '-';

    
    res[4] = (file_info->st_mode & S_IRGRP) ? 'r' : '-';
    res[5] = (file_info->st_mode & S_IWGRP) ? 'w' : '-';
    res[6] = (file_info->st_mode & S_IXGRP) ? 'x' : '-';

    
    res[7] = (file_info->st_mode & S_IROTH) ? 'r' : '-';
    res[8] = (file_info->st_mode & S_IWOTH) ? 'w' : '-';
    res[9] = (file_info->st_mode & S_IXOTH) ? 'x' : '-';
    res[10] = '\0';

    return (error_msg){SUCCESS, "", ""};
}

error_msg print_file_info(const char *file_name) {
    if (!file_name) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "null pointer received"};
    }

    struct stat file_info;
    if (stat(file_name, &file_info) == -1) {
        return (error_msg){INPUT_FILE_ERROR, __func__, "stat failed"};
    }

    char rights[11];
    error_msg err = access_rights(rights, &file_info);
    if (err.type) return err;

    struct passwd *pw = getpwuid(file_info.st_uid);
    struct group *gr = getgrgid(file_info.st_gid);
    char *user_name = pw ? pw->pw_name : "unknown";
    char *group_name = gr ? gr->gr_name : "unknown";

    char time_str[20];
    format_time(file_info.st_mtime, time_str);

    
    printf("%s %2lu %s %s %6lu %s %lu %s\n",
           rights, (unsigned long)file_info.st_nlink, user_name, group_name,
           (unsigned long)file_info.st_size, time_str, (unsigned long)file_info.st_ino, file_name);

    return (error_msg){SUCCESS, "", ""};
}

error_msg process_directory(const char *dir_name) {
    if (!dir_name) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "null pointer received"};
    }

    DIR *dir = opendir(dir_name);
    if (!dir) {
        return (error_msg){INPUT_FILE_ERROR, __func__, "cannot open directory"};
    }

    printf("Directory: %s\n", dir_name);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_name, entry->d_name);

        error_msg err = print_file_info(full_path);
        if (err.type) {
            closedir(dir);
            return err;
        }
    }

    closedir(dir);
    return (error_msg){SUCCESS, "", ""};
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory1> [directory2 ...]\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        error_msg err = process_directory(argv[i]);
        if (err.type) {
            return print_error(err);
        }
        if (i < argc - 1) {
            printf("\n");
        }
    }

    return 0;
}