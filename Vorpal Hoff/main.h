#ifndef MAIN
#define MAIN

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
#include "devices.h"
#include "data.h"
#include <inttypes.h>
#include "FTDI_helper.h"

//////////////////// Variables and Defines ////////////////////////////////////////////////////////

//#define PIN_TX  0x01  /* Orange wire on FTDI cable */
//#define PIX_RX  0x02  /* Yellow */
//#define PIN_RTS 0x04  /* Green */
#define PIN_CTS 0x08  /* Brown */
#define PIN_DTR 0x10
//#define PIN_DSR 0x20
//#define PIN_DCD 0x40
//#define PIN_RI  0x80

// HM-10 OR HM-11 COMMANDS
#define HM_RESET "AT+RESET"
#define HM_ISP_LOW "AT+PIO30"
#define HM_ISP_HIGH "AT+PIO31"
#define HM_LPC_RESET_LOW "AT+PIO20"
#define HM_LPC_RESET_HIGH "AT+PIO21"

// LPC Commands
#define LPC_CHECK "?"
#define Synchronized "Synchronized\n"

// FTDI
#define FT_ATTEMPTS 5

#define MAX_SIZE 32768
#define MAX_SIZE_16 2048

#define PRINT 1
#define NO_PRINT 0
#define PARSE 1
#define NO_PARSE 0

#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15


struct write
{
	// Holds the raw hex data, straight from the file.
	uint8_t UUE_chunk_A[256];
	uint8_t UUE_chunk_B[256];

	int UUE_chunk_A_check_sum;
	int UUE_chunk_B_check_sum;

	int UUE_chunk_A_bytes_to_write;
	int UUE_chunk_B_bytes_to_write;
	
	int UUE_chunk_A_UUE_char_count;
	int UUE_chunk_B_UUE_char_count;

	int chunk_index;
	int bytes_loaded_A;
	int bytes_loaded_B;
	int bytes_written;

	// ISP uses RAM from 0x1000 017C to 0x1000 17F
	int32_t ram_address;

	// Flash address.
	int32_t Flash_address;
	int sectors_needed;
	int sector_to_write;
	int sector_index;
};

struct Data
{
	// Holds the raw hex data, straight from the file.
	uint8_t HEX_array[128000];
	int HEX_array_size;
};

// States for FTDI state machine.
typedef enum {
    FTDI_SM_OPEN,
   	FTDI_SM_RESET,
    FTDI_SM_CLOSE,
    FTDI_SM_ERROR,
} FTDI_state;

typedef enum  
{
	RESP_CMD_SUCCESS                   =  0,
	RESP_INVALID_COMMAND               =  1,
	RESP_SRC_ADDR_ERROR                =  2,
	RESP_DST_ADDR_ERROR                =  3,
	RESP_SRC_ADDR_NOT_MAPPED           =  4,
	RESP_DST_ADDR_NOT_MAPPED           =  5,
	RESP_COUNT_ERROR                   =  6,
	RESP_INVALID_SECTOR                =  7,
	RESP_SECTOR_NOT_BLANK              =  8,
	RESP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION  = 9,
	RESP_COMPARE_ERROR                 = 10,
	RESP_BUSY                          = 11,
	RESP_PARAM_ERROR                   = 12,
	RESP_ADDR_ERROR                    = 13,
	RESP_ADDR_NOT_MAPPED               = 14,
	RESP_CMD_LOCKED                    = 15,
	RESP_INVALID_CODE                  = 16,
	RESP_INVALID_BAUD_RATE             = 17,
	RESP_INVALID_STOP_BIT              = 18,
	RESP_CODE_READ_PROTECTION_ENABLED  = 19
}CMD_RESPONSE;

// States for RX state machine.
typedef enum {
	RX_NODATA,
	RX_INCOMPLETE,
	RX_COMPLETE,
	RX_COMP_AND_INC_DATA,
	RX_ERROR,
} RX_state;

//////////////////// Forward Declaration ////////////////////////////////////////////////////////////

void main_menu();
void shut_down();

void program_chip();

// FTDI
uint8_t rx(bool parse, bool printOrNot);
uint8_t parserx();
uint8_t tx_chars(char string[], int txString_size, bool printOrNot, int frequency_of_tx_char);
uint8_t tx_data(uint8_t string[], int txString_size, bool printOrNot, int frequency_of_tx_char);
int FTDI_State_Machine(int state, int FT_Attempts);

// Output files, for debugging purposes.
void writeUUEDataTofile(uint8_t UUE_Encoded_String[], int hexDataCharCount);
void writeHexDataTofile(struct Data data_local);
FILE *open_file ( uint8_t *file, uint8_t *mode );


void OK();
void Failed();

// Debugging and Print Handling.
void clearBuffers();
void setTextRed();
void setTextGreen();
void setColor(int ForgC, int BackC);
void clearConsole();
void startScreen();

// Write to RAM.
int prepare_sectors(int sectors_needed);
int sectors_needed(int hex_data_array_size);
struct write write_page_to_ram(struct write write_local, struct Data data_local);
struct write prepare_page_to_write(struct write write_local, struct Data data_local);
struct write ram_to_flash(struct write write_local, struct Data data_local);
void convert_32_hex_address_to_string(uint32_t address, uint8_t * address_as_string);
struct write erase_chip(struct write write_local);
struct write validity_checksum(struct write write_local, struct Data data_local);

// Timers
float timer();
void usleep(__int64 usec);

#endif /* _INC_VORP-Q */