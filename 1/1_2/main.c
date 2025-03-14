#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

#define BLOCK_SIZE_2N(N) ((1 << N) / 8 ? (1 << N) / 8 : 1)

void countMaskedValues(int file_count, char *files[], uint32_t mask) {
    int current_file_index = 0;
    while (current_file_index < file_count) {
        FILE *file_pointer = fopen(files[current_file_index], "rb");
        if (file_pointer == NULL) {
            perror("Could not open file");
            current_file_index = current_file_index + 1;
            continue;
        }

        int total_matches = 0;
        uint32_t current_value;
        size_t number_of_bytes_read;

        while (1) {
            number_of_bytes_read = fread(&current_value, 1, sizeof(uint32_t), file_pointer);
            if (number_of_bytes_read != sizeof(uint32_t)) {
                break;
            }
            if (ferror(file_pointer)) {
                perror("Reading file failed");
                break;
            }
            uint32_t masked_result = current_value & mask;
            if (masked_result == mask) {
                total_matches = total_matches + 1;
            }
        }

        if (number_of_bytes_read > 0 && !feof(file_pointer)) {
            printf("Notice: %s ends with an incomplete 4-byte section\n", files[current_file_index]);
        }
        printf("%s contains %d matches\n", files[current_file_index], total_matches);
        fclose(file_pointer);
        current_file_index = current_file_index + 1;
    }
}

void bitwiseCombineN(int file_count, char *files[], int N) {
    size_t block_size_value = BLOCK_SIZE_2N(N);
    uint8_t *block_memory = calloc(block_size_value, 1);
    if (block_memory == NULL) {
        perror("Cannot allocate memory for block");
        return;
    }

    int file_index = 0;
    while (file_index < file_count) {
        FILE *file_handle = fopen(files[file_index], "rb");
        if (file_handle == NULL) {
            perror("Unable to access file");
            file_index = file_index + 1;
            continue;
        }

        uint8_t *result_memory = calloc(block_size_value, 1);
        if (result_memory == NULL) {
            perror("Memory allocation for result failed");
            fclose(file_handle);
            file_index = file_index + 1;
            continue;
        }

        size_t bytes_read_into_buffer;
        int number_of_blocks_processed = 0;

        while (1) {
            bytes_read_into_buffer = fread(block_memory, 1, block_size_value, file_handle);
            if (bytes_read_into_buffer == 0) {
                break;
            }
            if (ferror(file_handle)) {
                perror("File read error occurred");
                break;
            }
            if (bytes_read_into_buffer < block_size_value) {
                size_t remaining_bytes = block_size_value - bytes_read_into_buffer;
                memset(block_memory + bytes_read_into_buffer, 0, remaining_bytes);
            }
            if (number_of_blocks_processed == 0) {
                for (size_t index = 0; index < block_size_value; index++) {
                    result_memory[index] = block_memory[index];
                }
            } else {
                for (size_t index = 0; index < block_size_value; index++) {
                    result_memory[index] = result_memory[index] ^ block_memory[index];
                }
            }
            number_of_blocks_processed = number_of_blocks_processed + 1;
        }

        if (number_of_blocks_processed == 0) {
            printf("No data in %s\n", files[file_index]);
        } else {
            printf("Computed XOR for %s: ", files[file_index]);
            for (size_t index = 0; index < block_size_value; index++) {
                printf("%02x", result_memory[index]);
            }
            printf("\n");
        }

        free(result_memory);
        fclose(file_handle);
        file_index = file_index + 1;
    }
    free(block_memory);
}

void searchTextInFiles(int file_count, char *files[], const char *search_string) {
    if (strlen(search_string) == 0) {
        printf("Error: Empty search string provided.\n");
        return;
    }

    pid_t *pids = malloc(file_count * sizeof(pid_t));
    if (!pids) {
        perror("Cannot allocate memory for process IDs");
        return;
    }
    int pid_count = 0;
    int found = 0;

    for (int i = 0; i < file_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            FILE *file = fopen(files[i], "r");
            if (!file) {
                exit(1);
            }

            char *line = NULL;
            size_t len = 0;
            ssize_t read;
            int status = 2;
            while ((read = getline(&line, &len, file)) != -1) {
                if (ferror(file)) {
                    perror("File reading issue detected");
                    free(line);
                    fclose(file);
                    exit(3);
                }
                if (strstr(line, search_string)) {
                    printf("Match located in: %s\n", files[i]);
                    status = 0;
                    break;
                }
            }
            free(line);
            fclose(file);
            exit(status);
        } else if (pid < 0) {
            perror("Process creation failed");
            while (pid_count > 0) {
                waitpid(pids[--pid_count], NULL, 0);
            }
            free(pids);
            return;
        } else {
            pids[pid_count++] = pid;
        }
    }

    for (int i = 0; i < pid_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            found = 1;
        }
    }

    if (!found) {
        printf("No occurrences of '%s' found in the files.\n", search_string);
    }
    free(pids);
}

void replicateFilesN(int file_count, char *files[], int N) {
    pid_t *pids_array = malloc(file_count * N * sizeof(pid_t));
    if (pids_array == NULL) {
        perror("PID memory allocation error");
        return;
    }
    int number_of_pids = 0;

    int outer_loop_index = 0;
    while (outer_loop_index < file_count) {
        int inner_loop_index = 0;
        while (inner_loop_index < N) {
            pid_t current_pid = fork();
            if (current_pid == 0) {
                char new_filename[256];
                snprintf(new_filename, sizeof(new_filename), "%s_%d", files[outer_loop_index], inner_loop_index + 1);
                FILE *source_file = fopen(files[outer_loop_index], "rb");
                if (source_file == NULL) {
                    perror("Source file cannot be opened");
                    exit(1);
                }
                FILE *destination_file = fopen(new_filename, "wb");
                if (destination_file == NULL) {
                    perror("Destination file creation failed");
                    fclose(source_file);
                    exit(2);
                }

                uint8_t buffer[4096];
                size_t bytes_read;
                int exit_status = 0;
                while (1) {
                    bytes_read = fread(buffer, 1, sizeof(buffer), source_file);
                    if (bytes_read == 0) {
                        break;
                    }
                    if (ferror(source_file)) {
                        perror("Error reading source file");
                        exit_status = 3;
                        break;
                    }
                    if (fwrite(buffer, 1, bytes_read, destination_file) != bytes_read) {
                        perror("Writing to destination file failed");
                        exit_status = 4;
                        break;
                    }
                }
                fclose(source_file);
                fclose(destination_file);
                exit(exit_status);
            } else if (current_pid < 0) {
                perror("Fork operation unsuccessful");
                while (number_of_pids > 0) {
                    waitpid(pids_array[--number_of_pids], NULL, 0);
                }
                free(pids_array);
                return;
            } else {
                pids_array[number_of_pids++] = current_pid;
            }
            inner_loop_index = inner_loop_index + 1;
        }
        outer_loop_index = outer_loop_index + 1;
    }

    int failure_flag = 0;
    int wait_index = 0;
    while (wait_index < number_of_pids) {
        int status_code;
        waitpid(pids_array[wait_index], &status_code, 0);
        if (WIFEXITED(status_code) && WEXITSTATUS(status_code) != 0) {
            failure_flag = 1;
        }
        wait_index = wait_index + 1;
    }
    if (failure_flag) {
        printf("Certain copy tasks encountered errors.\n");
    }
    free(pids_array);
}

void showHelp() {
    printf("\n=== File Processor Usage ===\n");
    printf("Command: ./file_processor <file1> <file2> ... <flag> <args>\n");
    printf("\nAvailable Flags:\n");
    printf("---------------------------------------------------------\n");
    printf("| %-8s | %-42s |\n", "Flag", "Description");
    printf("---------------------------------------------------------\n");
    printf("| %-8s | %-42s |\n", "xorN <N>", "XOR blocks of 2^N bits (N=2,3,4,5,6)");
    printf("| %-8s | %-40s |\n", "mask <hex>", "Count 4-byte integers matching the mask");
    printf("| %-8s | %-41s |\n", "copyN <N>", "Create N copies of each file");
    printf("| %-8s | %-37s |\n", "find <string>", "Search for a string in files");
    printf("---------------------------------------------------------\n");
    printf("\nTip: Provide at least one file and a flag with its argument.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        showHelp();
        return 1;
    }

    int file_count = argc - 3;
    char *flag = argv[argc - 2];
    char *arg = argv[argc - 1];

    if (strcmp(flag, "xorN") == 0) {
        int N = atoi(arg);
        if (N < 2 || N > 6) {
            printf("Error: N for xorN must be between 2 and 6.\n");
            return 1;
        }
        bitwiseCombineN(file_count, argv + 1, N);
    } else if (strcmp(flag, "mask") == 0) {
        char *endptr;
        uint32_t mask = strtoul(arg, &endptr, 16);
        if (*endptr != '\0') {
            printf("Error: Invalid hexadecimal mask: %s\n", arg);
            return 1;
        }
        countMaskedValues(file_count, argv + 1, mask);
    } else if (strcmp(flag, "copyN") == 0) {
        int N = atoi(arg);
        if (N <= 0) {
            printf("Error: N for copyN must be a positive number.\n");
            return 1;
        }
        replicateFilesN(file_count, argv + 1, N);
    } else if (strcmp(flag, "find") == 0) {
        searchTextInFiles(file_count, argv + 1, arg);
    } else {
        printf("Unrecognized flag: %s\n", flag);
        showHelp();
        return 1;
    }

    return 0;
}