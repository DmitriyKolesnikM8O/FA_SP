#pragma once

#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <time.h>

#include "errors.h"

// Ручное определение констант
#define PATH_MAX 4096
#define S_IFIFO  0x1000  // FIFO (именованный канал)
#define S_IFCHR  0x2000  // Символьное устройство
#define S_IFBLK  0x6000  // Блочное устройство
#define S_IFSOCK 0xC000  // Сокет

error_msg processing_catalog(const char* catalog_name);
error_msg processing_file(const char* file_name, char* result);