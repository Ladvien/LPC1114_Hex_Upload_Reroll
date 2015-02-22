#ifndef DATA
#define DATA

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 
#include <windows.h>
#include <windef.h>
#include <winnt.h>
#include <winbase.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "ftd2xx.h"
#include <sys/time.h>
#include "main.h"
#include "devices.h"

int file_sizer(FILE *file_to_size);

// Data Handling
int hex_file_to_array(FILE * file, uint8_t * hex_data, int hex_array_size);
int make_array_multiple_of_four(uint8_t * hex_data, int hex_array_size);
int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size);
int check_sum(uint8_t * hex_data_array, int file_size);
uint8_t read_byte_from_file(FILE * file);
void clear_special_char(FILE * file);
uint8_t Ascii2Hex(uint8_t c);
void copy_string(uint8_t *target, uint8_t *source);
int tx_size_unsigned(uint8_t * string);
int tx_size_signed(char * string);

void decode_three(uint8_t * ret, char c0, char c1, char c2, char c3);

#endif /* DEVICES */