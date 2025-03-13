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
        perror("Failed to allocate memory for block");
        return; // Выход без утечки памяти
    }

    for (int i = 0; i < file_count; i++) {
        FILE *file = fopen(files[i], "rb");
        if (!file) {
            perror("Failed to open file");
            continue;
        }

        uint8_t *result = calloc(block_size, 1);
        if (!result) {
            perror("Failed to allocate memory for result");
            fclose(file);
            continue;
        }

        size_t bytes_read;
        int blocks_processed = 0;

        while ((bytes_read = fread(block, 1, block_size, file)) > 0) {
            if (bytes_read < block_size) {
                memset(block + bytes_read, 0, block_size - bytes_read); // Заполняем 0x00
            }
            if (blocks_processed == 0) {
                memcpy(result, block, block_size);
            } else {
                for (size_t j = 0; j < block_size; j++) {
                    result[j] ^= block[j];
                }
            }
            blocks_processed++;
        }

        if (blocks_processed == 0) {
            printf("File %s is empty\n", files[i]);
        } else {
            printf("XOR result for %s: ", files[i]);
            for (size_t j = 0; j < block_size; j++) {
                printf("%02x", result[j]);
            }
            printf("\n");
        }

        free(result);
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
        size_t bytes_read;
        while ((bytes_read = fread(&value, 1, sizeof(uint32_t), file)) == sizeof(uint32_t)) {
            if ((value & mask) == mask) {
                count++;
            }
        }
        if (bytes_read > 0) {
            printf("Warning: File %s has incomplete 4-byte block at end\n", files[i]);
        }
        printf("File %s: %d matches\n", files[i], count);
        fclose(file);
    }
}

void copyN_operation(int file_count, char *files[], int N) {
    pid_t *pids = malloc(file_count * N * sizeof(pid_t));
    if (!pids) {
        perror("Failed to allocate memory for PIDs");
        return;
    }
    int pid_count = 0;

    for (int i = 0; i < file_count; i++) {
        for (int j = 0; j < N; j++) {
            pid_t pid = fork();
            if (pid == 0) { // Дочерний процесс
                char new_filename[256];
                snprintf(new_filename, sizeof(new_filename), "%s_%d", files[i], j + 1);
                FILE *src = fopen(files[i], "rb");
                if (!src) {
                    perror("Failed to open source file");
                    exit(EXIT_FAILURE);
                }
                FILE *dst = fopen(new_filename, "wb");
                if (!dst) {
                    perror("Failed to open destination file");
                    fclose(src);
                    exit(EXIT_FAILURE);
                }

                uint8_t buffer[4096];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    if (fwrite(buffer, 1, bytes, dst) != bytes) {
                        perror("Failed to write to destination file");
                        fclose(src);
                        fclose(dst);
                        exit(EXIT_FAILURE);
                    }
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
    if (!pids) {
        perror("Failed to allocate memory for PIDs");
        return;
    }
    int found = 0;

    for (int i = 0; i < file_count; i++) {
        pid_t pid = fork();
        if (pid == 0) { // Дочерний процесс
            FILE *file = fopen(files[i], "r");
            if (!file) {
                exit(EXIT_FAILURE); // Тихий выход при ошибке открытия
            }

            char *line = NULL;
            size_t len = 0;
            ssize_t read;
            while ((read = getline(&line, &len, file)) != -1) {
                if (strstr(line, search_string)) {
                    printf("Found in file: %s\n", files[i]);
                    free(line);
                    fclose(file);
                    exit(EXIT_SUCCESS);
                }
            }
            if (read == -1 && !feof(file)) {
                perror("Error reading file");
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
        xorN_operation(file_count, argv + 1, N);
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