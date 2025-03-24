#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdint.h>

#define BLOCK_SIZE_2N(N) ((1 << N) / 8 ? (1 << N) / 8 : 1)

int countMaskedValues(int file_count, char *files[], uint32_t mask) {
    int current_file_index = 0;
    while (current_file_index < file_count) {
        FILE *file_pointer = fopen(files[current_file_index], "rb");
        if (file_pointer == NULL) {
            printf("Could not open file");
            current_file_index = current_file_index + 1;
            continue;
        }

        int total_matches = 0;
        uint32_t value;

        printf("Checking file %s with mask: 0x%08X\n", files[current_file_index], mask);

        while (fread(&value, sizeof(uint32_t), 1, file_pointer) == 1) {
            uint32_t masked_value = value & mask;
            printf("Value: 0x%08X, Masked: 0x%08X, Target: 0x%08X\n", 
                   value, masked_value, mask);
            if (masked_value == mask) {
                total_matches = total_matches + 1;
            }
        }

        printf("%s contains %d matches\n", files[current_file_index], total_matches);
        fclose(file_pointer);
        current_file_index = current_file_index + 1;
    }

    return 0;
}

int bitwiseCombineN(int file_count, char *files[], int N) {
    size_t block_size_value = BLOCK_SIZE_2N(N);
    uint8_t *block_memory = calloc(block_size_value, 1);
    if (block_memory == NULL) {
        printf("Cannot allocate memory for block");
        return 0;
    }

    int file_index = 0;
    while (file_index < file_count) {
        FILE *file_handle = fopen(files[file_index], "rb");
        if (file_handle == NULL) {
            printf("Unable to access file");
            file_index = file_index + 1;
            continue;
        }

        uint8_t *result_memory = calloc(block_size_value, 1);
        if (result_memory == NULL) {
            printf("Memory allocation for result failed");
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
                printf("File read error occurred");
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

    return 0;
}

int searchTextInFiles(int file_count, char *files[], const char *search_string) {
    if (strlen(search_string) == 0) {
        printf("Error: Empty search string provided.\n");
        return 0;
    }

    pid_t *pids = malloc(file_count * sizeof(pid_t));
    if (!pids) {
        printf("Cannot allocate memory for process IDs");
        return 0;
    }
    int pid_count = 0;

    
    for (int i = 0; i < file_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            
            FILE *file = fopen(files[i], "r");
            if (!file) {
                return -1;
            }

            char *line = NULL;
            size_t len = 0;
            while (getline(&line, &len, file) != -1) {
                if (strstr(line, search_string)) {
                    printf("Match located in: %s\n", files[i]);
                    free(line);
                    fclose(file);
                    return -1;
                }
            }
            free(line);
            fclose(file);
            return -1;
        } else if (pid < 0) {
            printf("Process creation failed");
        } else {
            pids[pid_count++] = pid;
        }
    }

    
    int remaining = pid_count;
    int found = 0;
    while (remaining > 0) {
        for (int i = 0; i < pid_count; i++) {
            if (pids[i] > 0) {
                int status;
                pid_t result = waitpid(pids[i], &status, WNOHANG);
                if (result > 0) {
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        found = 1;
                    }
                    pids[i] = -1;  
                    remaining--;
                }
            }
        }
        usleep(1000);  
    }

    if (!found) {
        printf("No occurrences of '%s' found in the files.\n", search_string);
    }
    free(pids);
    return 0;
}

int replicateFilesN(int file_count, char *files[], int N) {
    int total_processes = file_count * N;
    pid_t *pids = malloc(total_processes * sizeof(pid_t));
    if (!pids) {
        printf("PID memory allocation error");
        return 0;
    }
    int pid_count = 0;

    for (int file_idx = 0; file_idx < file_count; file_idx++) {
        for (int copy_idx = 0; copy_idx < N; copy_idx++) {
            pid_t pid = fork();
            if (pid == 0) {
                char *new_filename = NULL;
                char *dot = strrchr(files[file_idx], '.');
                size_t name_length;
                
                if (dot) {
                    name_length = dot - files[file_idx];
                    new_filename = malloc(name_length + 20); 
                    if (!new_filename) {
                        return -1;
                    }
                    strncpy(new_filename, files[file_idx], name_length);
                    new_filename[name_length] = '\0';
                    snprintf(new_filename + name_length, 20, "_%d%s", copy_idx + 1, dot);
                } else {
                    name_length = strlen(files[file_idx]);
                    new_filename = malloc(name_length + 10); 
                    if (!new_filename) {
                        return -1;
                    }
                    snprintf(new_filename, name_length + 10, "%s_%d", files[file_idx], copy_idx + 1);
                }
                
                FILE *source = fopen(files[file_idx], "rb");
                if (!source) {
                    free(new_filename);
                    return -1;
                }
                FILE *dest = fopen(new_filename, "wb");
                if (!dest) {
                    free(new_filename);
                    fclose(source);
                    return -1;
                }

                uint8_t buffer[4096];
                size_t bytes;
                while ((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
                    if (fwrite(buffer, 1, bytes, dest) != bytes) {
                        free(new_filename);
                        fclose(source);
                        fclose(dest);
                        return 3;
                    }
                }

                free(new_filename);
                fclose(source);
                fclose(dest);
                return -1;
            } else if (pid > 0) {
                pids[pid_count++] = pid;
            }
        }
    }

    int remaining = pid_count;
    int failures = 0;
    while (remaining > 0) {
        for (int i = 0; i < pid_count; i++) {
            if (pids[i] > 0) {
                int status;
                pid_t result = waitpid(pids[i], &status, WNOHANG);
                if (result > 0) {
                    if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                        failures++;
                    }
                    pids[i] = -1;
                    remaining--;
                }
            }
        }
        usleep(1000);
    }

    if (failures > 0) {
        printf("Some copy operations failed (%d failures)\n", failures);
    }
    free(pids);
    return 0;
}

int showHelp() {
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

    return 0;
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