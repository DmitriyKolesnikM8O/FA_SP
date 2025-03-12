#ifndef LAB4_ERRORS_H
#define LAB4_ERRORS_H

#include <stdio.h>

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

int print_error(error_msg error);


#endif