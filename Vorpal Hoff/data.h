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

int fileSizer();

// Data Handling
struct Data hex_file_to_array(struct Data data_local, int file_size);
int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size);
int check_sum(uint8_t * hex_data_array, int file_size);
uint8_t readByte();
void clearSpecChar();
static uint8_t Ascii2Hex(uint8_t c);
void copy_string(uint8_t *target, uint8_t *source);
int tx_size(uint8_t * string);

#endif /* DEVICES */