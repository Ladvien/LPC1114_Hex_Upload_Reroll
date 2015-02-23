#ifndef DATA
#define DATA

/*------------------------------------------------- DATA.h ----------
|  Copyright (c) 2015 C. Thomas Brittain
|							aka, Ladvien
|
|  Module: data 
|
|  Purpose: Common needed data handling funcitons to assist    
| 			with peripheral interactions   
|
|  Functions: FUNCTION: IN / OUT
|      
|              
|                      
|                       
|                       
|                       
|                       
|
|  Returns:  
*-------------------------------------------------------------------*/


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

#define MAX_SIZE 32768
#define MAX_SIZE_16 2048

// Get char count in file.
int file_sizer(FILE *file_to_size);

// Data Handling
int hex_file_to_array(FILE * file, uint8_t * hex_data, int hex_array_size);

// Makes an array mutiple of four; padding with 0xFF.
int make_array_multiple_of_four(uint8_t * hex_data, int hex_array_size);

// UUEncode a hex array.
int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size);

// Decode 4 characters from UUE array, convert them to 3 bytes.
void decode_three(uint8_t * ret, char c0, char c1, char c2, char c3);

// Get checksum of an array.
int check_sum(uint8_t * hex_data_array, int file_size);

// Get two chars from hex file, convert them to one byte.
uint8_t read_byte_from_file(FILE * file, uint8_t * charToPut, int * totalCharsRead);

// Get rid of special characters in hex file (e.g., /r /n)
void clear_special_char(FILE * file, uint8_t * charToPut, int * totalCharsRead);

// Convert ASCII character to byte.
uint8_t Ascii2Hex(uint8_t c);

// Homemade string copy.
void copy_string(uint8_t *target, uint8_t *source);

// Get the size of an UNsigned array.
int tx_size_unsigned(uint8_t * string);

// Get the size of a signed array.
int tx_size_signed(char * string);

#endif /* DATA */