#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <time.h>

// #define PATH_MAX 4096
// #define S_IFIFO  0x1000  // FIFO (именованный канал)
// #define S_IFCHR  0x2000  // Символьное устройство
// #define S_IFBLK  0x6000  // Блочное устройство
// #define S_IFSOCK 0xC000  // Сокет

typedef enum type_error{
    SUCCESS,
    INPUT_FILE_ERROR,
    OUTPUT_FILE_ERROR,
    MEMORY_ALLOCATED_ERROR,
    INDEX_VECTOR_ERROR,
    OVERFLOW_ERROR,
    INCORRECT_OPTIONS_ERROR,
    UNRECOGNIZED_CHARACTER_ERROR,
	NUMERAL_SYSTEM_ERROR,
	INCORRECT_ARG_FUNCTION,
	SEM_ERROR,
	FORK_ERROR,
	TOKEN_ERROR
} type_error;

typedef struct error_msg{
	type_error type;
	const char * func;
	const char * msg;
}error_msg;

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

error_msg access_rights(char* res, struct stat* file_info) {
    if (res == NULL || file_info == NULL) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "get pointer to null"};
    }
    char s1[] = "-dplcbs";
    int r1 = S_ISDIR(file_info->st_mode) % 2 + (S_IFIFO & file_info->st_mode) % 2 * 2 +
             S_ISLNK(file_info->st_mode) % 2 * 3 + (S_IFCHR & file_info->st_mode) % 2 * 4 +
             (S_IFBLK & file_info->st_mode) % 2 * 5 + (S_IFSOCK & file_info->st_mode) % 2 * 6;
    res[0] = s1[r1];

    // Права пользователя
    if (S_IRUSR & file_info->st_mode) res[1] = 'r'; else res[1] = '-';
    if (S_IWUSR & file_info->st_mode) res[2] = 'w'; else res[2] = '-';
    if (S_IXUSR & file_info->st_mode) res[3] = 'x'; else res[3] = '-';

    // Права группы пользователей
    if (S_IRGRP & file_info->st_mode) res[4] = 'r'; else res[4] = '-';
    if (S_IWGRP & file_info->st_mode) res[5] = 'w'; else res[5] = '-';
    if (S_IXGRP & file_info->st_mode) res[6] = 'x'; else res[6] = '-';

    // Права всех прочих
    if (S_IROTH & file_info->st_mode) res[7] = 'r'; else res[7] = '-';
    if (S_IWOTH & file_info->st_mode) res[8] = 'w'; else res[8] = '-';
    if (S_IXOTH & file_info->st_mode) res[9] = 'x'; else res[9] = '-';

    return (error_msg){SUCCESS, "", ""};
}

error_msg processing_file(const char* file_name, char* result) {
    if (file_name == NULL) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "get pointer to null"};
    }

    struct stat file_info;
    int er = stat(file_name, &file_info);
    if (er == -1) {
        return (error_msg){INPUT_FILE_ERROR, __func__, "stat file"};
    }

    char rights[11] = "\0";
    memset(rights, '\0', 10);
    error_msg errorMsg = access_rights(rights, &file_info);
    if (errorMsg.type) {
        return errorMsg;
    }
    rights[10] = '\0';
    sprintf(result, "%s ", rights);
    sprintf(result + strlen(result), "%2lu %s %s %6lu", file_info.st_nlink, 
            getpwuid(file_info.st_uid)->pw_name, getgrgid(file_info.st_gid)->gr_name, file_info.st_size);

    char mtime_str[80];
    format_time(file_info.st_mtime, mtime_str);
    sprintf(result + strlen(result), " %s", mtime_str);

    char abs_path[PATH_MAX];
    if (realpath(file_name, abs_path) == NULL) {
        return (error_msg){INPUT_FILE_ERROR, __func__, "realpath"};
    }
    sprintf(result + strlen(result), " %s\n", abs_path);

    return (error_msg){SUCCESS, "", ""};
}


error_msg processing_catalog(const char* catalog_name) {
    if (catalog_name == NULL) {
        return (error_msg){INCORRECT_ARG_FUNCTION, __func__, "get pointer to null"};
    }

    DIR* dir = opendir(catalog_name);
    if (dir == NULL) {
        return (error_msg){INPUT_FILE_ERROR, __func__, "the catalog didn't open"};
    }
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char info[PATH_MAX * 2] = "\0";
        memset(info, '\0', PATH_MAX * 2);

        // Формируем полный путь
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", catalog_name, entry->d_name);

        error_msg errorMsg = processing_file(full_path, info); // Передаём полный путь
        if (errorMsg.type) {
            closedir(dir);
            return errorMsg;
        }
        printf("%s", info);
    }

    closedir(dir);
    return (error_msg){SUCCESS, "", ""};
}

int main(int argc, char* argv[]) {
	for (int j = 1;j < argc; ++j) {
		char * catalog_name = argv[j];
		error_msg errorMsg = processing_catalog(catalog_name);
		if(errorMsg.type){
			return print_error(errorMsg);
		}
	}
	return 0;
}