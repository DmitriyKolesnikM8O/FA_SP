#include <stdio.h>
#include <stdint.h>

int main() {
    FILE *file = fopen("test.bin", "wb");
    if (!file) {
        perror("Не удалось создать файл");
        return 1;
    }
    
    
    uint32_t values[] = {
        0x00000001,  // только бит 0
        0x00000003,  // биты 0 и 1
        0x00000005,  // биты 0 и 2
        0x0000000F,  // биты 0, 1, 2, 3
        0x00000010,  // только бит 4
        0x00000011,  // биты 0 и 4
        0x0000FF00,  // байт 2
        0x00FF0000,  // байт 3
        0xFF000000   // байт 4
    };
    
    for (int i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        fwrite(&values[i], sizeof(uint32_t), 1, file);
    }
    
    fclose(file);
    printf("Файл test.bin создан с 9 тестовыми числами\n");
    
    return 0;
} 