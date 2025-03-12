#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

#define BLOCK_SIZE_2N(N) ((1 << N) / 8 ? (1 << N) / 8 : 1) // 2^N бит = 2^(N-3) байт

void xorN_operation(int file_count, char *files[], int N) {
    size_t block_size = BLOCK_SIZE_2N(N);
    uint8_t *block = calloc(block_size, 1);
    if (!block) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < file_count; i++) {
        FILE *file = fopen(files[i], "rb");
        if (!file) {
            perror("Failed to open file");
            continue;
        }

        uint8_t result[block_size];
        memset(result, 0, block_size);
        size_t bytes_read;
        int first_block = 1;

        while ((bytes_read = fread(block, 1, block_size, file)) > 0) {
            if (bytes_read < block_size) {
                memset(block + bytes_read, 0, block_size - bytes_read);
            }
            if (first_block) {
                memcpy(result, block, block_size);
                first_block = 0;
            } else {
                for (size_t j = 0; j < block_size; j++) {
                    result[j] ^= block[j];
                }
            }
        }

        printf("XOR result for %s: ", files[i]);
        for (size_t j = 0; j < block_size; j++) {
            printf("%02x", result[j]);
        }
        printf("\n");

        fclose(file);
    }
    free(block);
}

void mask_operation(int file_count, char *files[], uint32_t mask) {
    for (int i = 0; i < file_count; i++) {
        FILE *file = fopen(files[i], "rb");
        if (!file) {
            perror("Failed to open file");
            continue;
        }

        int count = 0;
        uint32_t value;
        while (fread(&value, sizeof(uint32_t), 1, file) == 1) {
            if ((value & mask) == mask) {
                count++;
            }
        }
        printf("File %s: %d matches\n", files[i], count);
        fclose(file);
    }
}

void copyN_operation(int file_count, char *files[], int N) {
    pid_t *pids = malloc(file_count * N * sizeof(pid_t));
    int pid_count = 0;

    for (int i = 0; i < file_count; i++) {
        for (int j = 0; j < N; j++) {
            pid_t pid = fork();
            if (pid == 0) {
                char new_filename[256];
                snprintf(new_filename, sizeof(new_filename), "%s_%d", files[i], j + 1);
                FILE *src = fopen(files[i], "rb");
                FILE *dst = fopen(new_filename, "wb");
                if (!src || !dst) {
                    perror("Failed to open file");
                    if (src) fclose(src);
                    if (dst) fclose(dst);
                    exit(EXIT_FAILURE);
                }

                uint8_t buffer[4096];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytes, dst);
                }

                fclose(src);
                fclose(dst);
                exit(EXIT_SUCCESS);
            } else if (pid < 0) {
                perror("Failed to fork");
            } else {
                pids[pid_count++] = pid;
            }
        }
    }

    for (int i = 0; i < pid_count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    free(pids);
}

void find_operation(int file_count, char *files[], const char *search_string) {
    pid_t *pids = malloc(file_count * sizeof(pid_t));
    int found = 0;

    for (int i = 0; i < file_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            FILE *file = fopen(files[i], "r");
            if (!file) {
                exit(EXIT_FAILURE); // Не выводим ошибку, просто выходим
            }

            char *line = NULL;
            size_t len = 0;
            while (getline(&line, &len, file) != -1) {
                if (strstr(line, search_string)) {
                    printf("Found in file: %s\n", files[i]);
                    free(line);
                    fclose(file);
                    exit(EXIT_SUCCESS);
                }
            }

            free(line);
            fclose(file);
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            perror("Failed to fork");
        } else {
            pids[i] = pid;
        }
    }

    for (int i = 0; i < file_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
            found = 1;
        }
    }

    if (!found) {
        printf("String '%s' not found in any file.\n", search_string);
    }
    free(pids);
}

void print_usage() {
    printf("Usage: ./file_processor <file1> <file2> ... <flag> <args>\n");
    printf("Flags:\n");
    printf("  xorN <N> - XOR blocks of 2^N bits (N=2,3,4,5,6)\n");
    printf("  mask <hex> - Count 4-byte integers matching the mask\n");
    printf("  copyN <N> - Create N copies of each file\n");
    printf("  find <string> - Search for a string in files\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) { // Минимум: имя программы, 1 файл, флаг, аргумент
        print_usage();
        return 1;
    }

    int file_count = argc - 3; // Количество файлов
    char *flag = argv[argc - 2];
    char *arg = argv[argc - 1];

    if (strcmp(flag, "xorN") == 0) {
        int N = atoi(arg);
        if (N < 2 || N > 6) {
            printf("Invalid N for xorN. Must be 2-6.\n");
            return 1;
        }
        xorN_operation(file_count, argv + 1, N); // Пропускаем argv[0]
    } else if (strcmp(flag, "mask") == 0) {
        char *endptr;
        uint32_t mask = strtoul(arg, &endptr, 16);
        if (*endptr != '\0') {
            printf("Invalid hex mask: %s\n", arg);
            return 1;
        }
        mask_operation(file_count, argv + 1, mask);
    } else if (strcmp(flag, "copyN") == 0) {
        int N = atoi(arg);
        if (N <= 0) {
            printf("Invalid N for copyN. Must be positive.\n");
            return 1;
        }
        copyN_operation(file_count, argv + 1, N);
    } else if (strcmp(flag, "find") == 0) {
        find_operation(file_count, argv + 1, arg);
    } else {
        printf("Unknown flag: %s\n", flag);
        print_usage();
        return 1;
    }

    return 0;
}