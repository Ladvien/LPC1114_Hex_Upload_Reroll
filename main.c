//LPC1114 Reset program.  Meant to come before using lpc21isp.  Should allow for 
//popular FTDI breakouts, like Sparkfun's, to be used as a serial programmer for 
//the LPC1114.  Shooting for no manual reset.

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

//////////////////// Forward Declaration ////////////////////////////////////////////////////////////

// Output files, for debugging purposes.
void writeUUEDataTofile(uint8_t UUE_Encoded_String[], int hexDataCharCount);
void writeHexDataTofile(uint8_t * fileData_Hex_String, int hexDataCharCount);

// FTDI
static FILE *open_file ( char *file, char *mode );
int fileSizer();
uint8_t rx(bool parse, bool printOrNot);
uint8_t parserx();
uint8_t txString(uint8_t string[], int txString_size, bool printOrNot, int frequency_of_tx_char);
int FTDI_State_Machine(int state, int FT_Attempts);
uint8_t get_LPC_Info(bool print);

void command_response();

// LPC handling
uint8_t set_ISP_mode(int print);

// Data Handling
int hex_file_to_array(uint8_t * hex_data_array, int file_size);
int check_sum(uint8_t * hex_data_array, int file_size);
int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size);


uint8_t readByte();
void clearSpecChar();
int Hex2Ascii(uint8_t hexValue);
int Hex2Int(uint8_t c);
static uint8_t Ascii2Hex(uint8_t c);


//struct UUE_Data UUencode(struct hexFile hexFile);
void clearBuffers();
int tx_size(uint8_t * string);

void setTextRed();
void setTextGreen();
void setColor(int ForgC, int BackC);
void clearConsole();
void copy_string(uint8_t *target, uint8_t *source);
void startScreen();
void OK();
void Failed();
void check_HM_10();
void wake_devices();

int write_two_pages_to_ram(uint8_t * uue_pages_or_scrap_array, uint8_t * ram_address, int uue_pages_or_scrap_char_count, int pages_or_scrap_check_sum, int number_of_bytes_to_write, int hex_data_array_size);

int uue_create_pages_or_scrap(uint8_t * uue_pages_or_scrap_array, uint8_t * hex_data_array, int hex_data_array_size, int * pages_or_scrap_check_sum, int * number_of_bytes_to_write);
void convert_32_hex_address_to_string(uint32_t address, uint8_t * address_as_string);
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


//Serial Port handle.
//Used by FTD2XX
FT_HANDLE handle = NULL;
FT_STATUS FT_status;
DWORD EventDWord;
DWORD TxBytes;
DWORD bytes;
DWORD RxBytes;
DWORD BytesReceived;
uint8_t RawRxBuffer[2048];
uint8_t ParsedRxBuffer[2048];

//File to be loaded.	
FILE *fileIn;
FILE *hexDataFile;
FILE *UUEDataFile;

//Reading characters from a file.
uint8_t charToPut;

//Total bytesRead.
int totalCharsRead = 0;

int command_response_code = 0;



struct writeToRam{
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





///////////////////// PROGRAM ////////////////////////////
////////////////////// START /////////////////////////////

int main(int argc, uint8_t *argv[])
{
	// Setup console.
	clearConsole();
	startScreen();	

	//For setting state of DTR/CTS.
	uint8_t DTR_Switch;
	uint8_t CTS_Switch;

	// Holds the raw hex data, straight from the file.
	uint8_t hex_data_array[MAX_SIZE];
	uint8_t UUE_data_array[MAX_SIZE];
	// Stores file size.
	int fileSize = 0;

	// Dimesions of write_to_ram data.
	uint8_t uue_pages_or_scrap_array[1024];
	int hex_data_array_size = 0;
	int UUE_data_array_size = 0;
	int uue_pages_or_scrap_char_count = 0;
	int number_of_bytes_to_write = 0;
	int pages_or_scrap_check_sum = 0;
	

	// Holds hex data checksum, divided into 
	// chunks of 900 bytes.
	long int check_sum_bfr = 0;

	// Local for FTDI State Machine.
	//FTDI_state FTDI_Operation = RX_CLOSE;

	//If the user fails to give us two arguments yell at him.	
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s <readfile1>\n", argv[0] );
		exit ( EXIT_FAILURE );
	}

	//Open file using command-line info; for reading.
	fileIn = open_file (argv[1], "rb" );
	
	// Open FTDI.
	FTDI_State_Machine(FTDI_SM_OPEN, FT_ATTEMPTS);

	// Strange, this has to happen to get a response from the device.
	FTDI_State_Machine(FTDI_SM_RESET, FT_ATTEMPTS);	 
	
	//Set up pins we will use.
	//FT_SetBitMode(handle, PIN_DTR | PIN_CTS, 1);
	//Setup serial port, even though we are banging.
	FT_SetBaudRate(handle, 9600);  //* Actually 9600 * 16

	// Sizes file to be used in data handling.
	fileSize = fileSizer();

	// Load the data from file
	hex_data_array_size = hex_file_to_array(hex_data_array, fileSize);
	
	// Get checksum int array.
	//check_sum(hex_data_array, fileSize, check_sum_bfr);
	
	// Write hex string back to a file.  Used for debugging.
	writeHexDataTofile(hex_data_array, hex_data_array_size);
	
	// DEBUG:
		
	// Convert hex data to UUE.
	//UUE_data_array_size = UUEncode(UUE_data_array, hex_data_array, hex_data_array_size);

	// Write the UUE string to a file.  CURRENTLY BROKEN-ish.
	writeUUEDataTofile(UUE_data_array, UUE_data_array_size);

	// UUEncode 2 pages (512 bytes).  Returns UUE character count (~1033)
	uue_pages_or_scrap_char_count = uue_create_pages_or_scrap(uue_pages_or_scrap_array, hex_data_array, hex_data_array_size, &pages_or_scrap_check_sum, &number_of_bytes_to_write);

	for (int i = 0; i < uue_pages_or_scrap_char_count; i)
	{
		for (int line_index = 0; line_index < 61; ++line_index)
		{
			//printf("%C", uue_pages_or_scrap_array[i]);
			i++;
		}
		printf("\n");
	}
	
	// Let's wake the device chain (FTDI, HM-10, HM-10, LPC)
	wake_devices();
	
	// Check the RSSI of HM-10.
	check_HM_10();

	// Clear the console color.
	clearConsole();
	
	// Set LPC into ISP mode.
	set_ISP_mode(NO_PRINT);

	// Get LPC Device info.
	//get_LPC_Info(NO_PRINT);

	//Sleep(500); 
	//txString(HM_RESET, sizeof(HM_RESET), PRINT, 0);
	
	// Send Unlock Code
	txString("U 23130\n", sizeof("U 23130\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, PRINT);
	
	uint8_t ram_address[5];
	ram_address[0] = 0x10;
	ram_address[1] = 0x00;
	ram_address[2] = 0x00;
	ram_address[3] = 0x00;
	ram_address[4] = '\n';

	write_two_pages_to_ram(uue_pages_or_scrap_array, ram_address, uue_pages_or_scrap_char_count, pages_or_scrap_check_sum, number_of_bytes_to_write, hex_data_array_size);
	/*
	// DEBUG NOTES:
	// It seems the hexFile.original_data_checksum printed from the hexRead function
	// does not equal what it does from the main function.
	char write_to_ram_string[64];
	
	// Turn UUE string size into ASCII.
	char tx_byte_count[64];
	
	//itoa(UUE_Data.UUE_Encoded_String_Index, tx_byte_count);
	sprintf(tx_byte_count, "%i", hexFile.fileData_Hex_String_Size);

	// Turn hex data checksum into ASCII.
	char tx_check_sum[64];
	sprintf(tx_check_sum, "%i", (hexFile.original_data_checksum[0]));
	
	printf("orig-d-check %i\n", hexFile.original_data_checksum);
	printf("tx_check_sum %s\n", tx_check_sum);
	printf("BYTE COUNT: %c\n", tx_byte_count[0]);

	sprintf(write_to_ram_string, "W 268436224 %c\n", tx_byte_count[0]);
	// Write memory
	txString(write_to_ram_string, sizeof(write_to_ram_string), PRINT, 5);
	Sleep(300);
	rx(PARSE, PRINT);
	Sleep(100);
	txString(UUE_Data.UUE_Encoded_String, UUE_Data.UUE_Encoded_String_Index, PRINT, 0); // Send UUE data
	txString("\n", sizeof("\n"), PRINT, 0); // End UUE data
	txString(tx_check_sum, sizeof(tx_check_sum), PRINT, 0); // Send hex data checksum.
	txString("\n", sizeof("\n"), PRINT, 0); // End checksum.

	Sleep(500);
	rx(PARSE, PRINT);

	//txString(hexFile.fileData_Hex_String, hexFile.fileData_Hex_String_Size, PRINT, 0);
	//rx(NO_PARSE, PRINT);
	//txString("226\n", sizeof("226\n"), PRINT, 0);
	//Sleep(120);
	//rx(NO_PARSE, PRINT);

	
	
*/



	/*
	// Read memory
	txString("R 268436224 4\n", sizeof("R 268436224 4\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, PRINT);
	txString("OK\n", sizeof("OK\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, PRINT);


		// Write memory
		txString("W 268436224 8\n", sizeof("W 268436224 8\n"), 0);
		Sleep(500);
		rx(NO000, rxString);

		txString("(5&AE(&-A<@\n403\n", sizeof("(5&AE(&-A<@\n403\n"), 0);
		
		// TEST
		// HEX: 54 68 65 20 63 61 72
		// DECL 84  104 101 32 99 97
		// UUE: 5&AE(&-A

		printf("THIS NUMBER: %i\n", "RESEND");

		Sleep(500);
		rx(NO000, rxString);

	// Read memory
	txString("R 268436224 4\n", sizeof("R 268436224 4\n"), 0);
	Sleep(500);
	rx(NO000, rxString);
	txString("OK\n", sizeof("OK\n"), 0);
	Sleep(500);
	rx(NO000, rxString);

	//txString("#0V%T\n", sizeof("#0V%T\n"), 0);
	//Sleep(500);
	//rx(NO000, rxString);
	*/

	//Close files.
	fclose ( fileIn );
	fclose ( UUEDataFile );
	fclose ( hexDataFile );

	clearConsole();
} // END PROGRAM

// Issues:
// 1. intent to write string needs to be derived from byte to write var.
// 2. Fix check_sum being an array.
// 3. Create a flow of functions for writing to ram.
// 4. Address the "M" in UUE data.
// 5. Assure when writing to RAM we are not writing unused array spaces.

int uue_create_pages_or_scrap(uint8_t * uue_pages_or_scrap_array, uint8_t * hex_data_array, int hex_data_array_size, int * pages_or_scrap_check_sum, int * number_of_bytes_to_write)
{
	// 0. Add a check to see if data is insufficient for two pages,
	// if so, divert to get_scrap_paper().
	// 1. Get 512 bytes of hex data (two pages).
	// 2. Create UUEncode array from hex pages.
	// 3. Create checksum for encoded pages.
	// 4. Return checksum and UUEncoded array.

	int uue_pages_or_scrap_char_count = 0;
	uint8_t hex_two_page_array[2048];
	int count = 0;



	if (hex_data_array_size < 512)
	{
		// Get scrap.
				// Get two pages.
		// 512 / .75 = 682.6666 ~ 686
		
		// 1. Get 512 bytes of hex data (two pages).
		for (int i = 0; i < hex_data_array_size; ++i)
		{
			hex_two_page_array[i] = hex_data_array[i];
		}

		*number_of_bytes_to_write = hex_data_array_size;
		// 2. Create UUEncode array from hex pages.
		uue_pages_or_scrap_char_count = UUEncode(uue_pages_or_scrap_array, hex_two_page_array, hex_data_array_size);

		// 3. Create checksum for encoded pages.
		*pages_or_scrap_check_sum = check_sum(hex_data_array, hex_data_array_size);

		// 4. Return checksum and UUEncoded array.
	}
	else
	{
		// Get two pages.
		// 512 / .75 = 682.6666 ~ 686
		
		// 1. Get 512 bytes of hex data (two pages).
		for (int i = 0; i < 512; ++i)
		{
			hex_two_page_array[i] = hex_data_array[i];
		}

		*number_of_bytes_to_write = 512;
		// 2. Create UUEncode array from hex pages.
		uue_pages_or_scrap_char_count = UUEncode(uue_pages_or_scrap_array, hex_two_page_array, 512);

		// 3. Create checksum for encoded pages.
		*pages_or_scrap_check_sum = check_sum(hex_data_array, 512);

		// 4. Return checksum and UUEncoded array.
	}
	
	return uue_pages_or_scrap_char_count;
}

int check_sum(uint8_t * hex_data_array, int hex_data_array_size)
{
	int check_sum = 0;
	int page_index = 0;
	int char_index = 0;

	while(char_index < hex_data_array_size)
	{
		check_sum += hex_data_array[char_index];
		char_index++;
	}
	return check_sum;
}

void convert_32_hex_address_to_string(uint32_t address, uint8_t * address_as_string)
{
	// 1. Divide 32 bit int into nybbles.
	// 2. Convert nybble into character.
	// 3. Place characters into string.
	int char_index = 0;
	char buf_nybble;
	uint32_t buf_address = address;

	// Loop through all 8 nybbles.
	while(char_index < 8)
	{
		buf_address = address;
		buf_nybble = ((buf_address << char_index*4) >> 28);
		buf_nybble = buf_nybble + '0';
		// If a letter, let's compensate.
		if (buf_nybble > 0x39)
		{
			buf_nybble = buf_nybble + 7;
		}
		address_as_string[char_index] = buf_nybble;
		char_index++;
	}
}

int write_two_pages_to_ram(uint8_t * uue_pages_or_scrap_array, uint8_t * ram_address, int uue_pages_or_scrap_char_count, int pages_or_scrap_check_sum, int number_of_bytes_to_write, int hex_data_array_size)
{
	// 1. Convert RAM address from hex to decimal, then, from decimal to ASCII.
	// 2. Create intent-to-write-to-ram string: "W 268435456 512\n"
	// 3. Send intent-to-write string.
	// 4. Send two pages of data: "DATA\n"
	// 5. Send checksum: "Chk_sum\n"
	// 6. Read response from LPC.
	// 7. Determine if write was a success.
	// 8. Repeat write if necessary.
	// 9. Return true if successful (combine step 9?)

	printf("hex_data_array_size %i\n", hex_data_array_size);
	printf("pages_or_scrap_check_sum %i\n", pages_or_scrap_check_sum);
	printf("number_of_bytes_to_write %i\n", number_of_bytes_to_write);
	printf("uue_pages_or_scrap_char_count %i\n", uue_pages_or_scrap_char_count);
	
	uint8_t address_as_string[9];
	uint32_t hex_ram_address = 0;
	long int dec_ram_address = 0;
	uint8_t dec_address_as_string[32];
	uint8_t intent_to_write_to_ram_string[128];
	uint8_t checksum_as_string[64];
	
	hex_ram_address = 0x10000000; // Test address.

	// 1. Convert RAM address from hex to decimal, then, from decimal to ASCII.
	convert_32_hex_address_to_string(hex_ram_address, address_as_string);
	dec_ram_address = strtol(address_as_string, NULL, 16);
	snprintf(dec_address_as_string, sizeof(dec_address_as_string), "%d", dec_ram_address);

	// 2. Create intent-to-write-to-ram string: "W 268435456 512\n"
	snprintf(intent_to_write_to_ram_string, sizeof(intent_to_write_to_ram_string), "W %s %i\n", dec_address_as_string, number_of_bytes_to_write);
	printf("%s\n", intent_to_write_to_ram_string);


	// 3. Send intent-to-write string.
	txString(intent_to_write_to_ram_string, tx_size(intent_to_write_to_ram_string), PRINT, 0);
	txString("\n", sizeof("\n"), PRINT, 0);
	Sleep(400);
	rx(NO_PARSE, PRINT);

	// 4. Send two pages of data: "DATA\n"
	txString(uue_pages_or_scrap_array, uue_pages_or_scrap_char_count+2, PRINT, 0);
	txString("\n", sizeof("\n"), PRINT, 0);
	Sleep(600);
	rx(NO_PARSE, PRINT);
	

	// 5. Send checksum: "Chk_sum\n"
	snprintf(checksum_as_string, 10, "%i\n", pages_or_scrap_check_sum);
	txString(checksum_as_string, tx_size(checksum_as_string), PRINT, 0);
	txString("\n", sizeof("\n"), PRINT, 0);
	Sleep(200);
	rx(NO_PARSE, PRINT);

}

int tx_size(uint8_t * string)
{
	int count_char_in_string = 0;
	while(string[count_char_in_string] != '\n'){count_char_in_string++;}
	count_char_in_string++;

	return count_char_in_string;
}
///////////// LPC Handling ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

uint8_t set_ISP_mode(int print)
{
	// Remotely set LPC into ISP mode.

	// Test completeness of command.
	int successful = 0;

	printf("Starting LPC ISP.");

	// Let's attempt 3x to successfully set ISP mode
	for (int i = 0; i < 5; ++i)
	{	
		txString(HM_ISP_LOW, sizeof(HM_ISP_LOW), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");

		txString(HM_LPC_RESET_LOW, sizeof(HM_LPC_RESET_LOW), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");
		
		txString(HM_LPC_RESET_HIGH, sizeof(HM_LPC_RESET_HIGH), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");

		txString(HM_ISP_HIGH, sizeof(HM_ISP_HIGH), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");
		
		// Synchronized check.
		txString(LPC_CHECK, sizeof(LPC_CHECK), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");

		// Tell the LPC we are synchronized.
		txString(Synchronized, sizeof(Synchronized), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");

		// Set crystal
		txString("12000\n", sizeof("12000\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		printf(".");

		// Let's turn off ECHO.
		txString("A 0\n", sizeof("A 0\n"), print, 10);
		Sleep(500);
		rx(PARSE, print);		
		printf(".");

		// Set baud
		//txString("9600\n", sizeof("9600\n"), 0);
		//Sleep(500);
		//setTextGreen();
		//printf("%s\n", ParsedRxBuffer);

		Sleep(100);

		// If all the HM-10 commands responded then
		// the ISP mode should successfully be set.
		if (successful > 6)
		{
			OK();
			return 1;	
		}
		// Let's try 3x.
		else
		{
			Failed();
			successful = 0;
			printf("Retrying.");
			clearBuffers();
			Sleep(500);
		}
	}
	// RETURN NOTHING!
	return 0;
}

void wake_devices()
{
	// Meant to activate UART flow on devices
	txString("Wake", sizeof("Wake"), NO_PRINT, 10);
	Sleep(500);
	rx(NO_PARSE, PRINT);
	printf("Waking Devices...\n");
}

void check_HM_10()
{
	// Determine if the HM-10 signal is strong enough
	// to attempt an upload.
	uint8_t char_RSSI[2];
	int int_RSSI = 0;

	// An HM-10 command to get RSSI.
	txString("AT+RSSI?", sizeof("AT+RSSI?"), NO_PRINT, 0);
	Sleep(500);

	FT_GetStatus(handle, &RxBytes, &TxBytes, &EventDWord);

	if (RxBytes > 0) {
		FT_status = FT_Read(handle,RawRxBuffer,RxBytes,&BytesReceived);
	}
	else
	{
		// Bad RSSI read.
	}		
	
	if(sizeof(RawRxBuffer) < 10)
	{
		char_RSSI[0] = 0;
		char_RSSI[1] = RawRxBuffer[8];
		char_RSSI[2] = RawRxBuffer[8];	
	}
	else if (sizeof(RawRxBuffer) > 9)
	{
		char_RSSI[0] = RawRxBuffer[8];
		char_RSSI[1] = RawRxBuffer[9];
		char_RSSI[2] = RawRxBuffer[10];	
	}
	
	int_RSSI = atoi(char_RSSI);
	//sscanf(char_RSSI, "%D", &int_RSSI);

	if (int_RSSI > 30 && int_RSSI < 60)
	{
 		clearConsole();
		printf("HM-10 signal: ");
		setTextGreen();
		printf("Strong\n");		
	}
	else if (int_RSSI > 60 && int_RSSI < 90)
	{
		clearConsole();
		printf("HM-10 signal: ");
		printf("Medium\n");	
	}
	else if (int_RSSI > 90 && int_RSSI < 120)
	{
		clearConsole();
		printf("HM-10 signal: ");
		setTextRed();
		printf("Weak\n");		
	}
	clearConsole();
}


uint8_t get_LPC_Info(bool print)
{
	int successful = 0;
	uint8_t PartID[256];
	uint8_t UID[256];
	uint8_t BootVersion[256];

	for (int i = 0; i < 3; ++i)
	{	
		// Read Part ID
		txString("N\n", sizeof("N\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(PartID, ParsedRxBuffer);
		
		// Read UID
		txString("J\n", sizeof("J\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(UID, ParsedRxBuffer);

		// Boot Version
		txString("K\n", sizeof("K\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(BootVersion, ParsedRxBuffer);
		

		clearConsole();

		if (successful > 2)
		{
			printf("Device info successfully read: \n\n");
			printf("Boot Version: %s\n", BootVersion);
			printf("UID:          %s\n", UID);
			printf("Part Number:  %s\n", PartID);
			break;	
		}
		else
		{
			successful = 0;
			printf("Attempt %i to get LPC info and failed. Retrying...%s\n", i);
			clearBuffers();
			Sleep(500);
		}
	}

}

void OK()
{
	setTextGreen();
	printf("OK.\n\n");
	clearConsole();
}
void Failed()
{
	setTextRed();
	printf("FAILED.\n\n");
	clearConsole();	
}



///////////// FTDI  //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

int FTDI_State_Machine(int state, int FT_Attempts)
{
	switch(state)
	{
		case FTDI_SM_OPEN: 
			// Loop for command attempts.
			for (int i = 0; i < FT_Attempts; ++i)
			{
				// FT command
				FT_Open(0, &handle);
				if (FT_status != FT_OK)
				{
					printf("Could not open FTDI device. Attempt %i\n", i);
					Sleep(100);
				}
				else
				{
					return 1;
				}
			}
		case FTDI_SM_RESET: 
			for (int i = 0; i < FT_Attempts; ++i)
			{
				FT_ResetPort(handle);
				if (FT_status != FT_OK)
				{
					printf("Could not reset FTDI device. Attempt %i\n", i);
					Sleep(100);
				}
				else
				{
					return 1;
				}
			}
		case FTDI_SM_CLOSE:
			for (int i = 0; i < FT_Attempts; ++i)
			{
				FT_Close(handle);
				if (FT_status != FT_OK)
				{
					printf("Could not close FTDI device. Attempt %i\n", i);
					Sleep(100);
				}
				else
				{
					return 1;
				}
			}
		case FTDI_SM_ERROR:
			printf("Error in FTDI SM call.");
			break;
	}
}

//Open file for reading, function.
static FILE *open_file ( char *file, char *mode )
{
  FILE *fileOpen = fopen ( file, mode );

  if ( fileOpen == NULL ) {
    perror ( "Unable to open file" );
    exit (EXIT_FAILURE);
  }

  return fileOpen;
}

int fileSizer()
{
	int fileSize;
	while((fgetc (fileIn)) != EOF)
	{
		fileSize++;
	}
	rewind(fileIn);
	return fileSize;
}


uint8_t rx(bool parse, bool printOrNot)
{	
	// 0 = unsuccessful, 1 = LPC, 2 = HM-10
	int device_and_success = 0;
	//FT_SetTimeouts(handle, timeout, 0);
	FT_GetStatus(handle, &RxBytes, &TxBytes, &EventDWord);

	setTextGreen();

	if (RxBytes > 0) {

		FT_status = FT_Read(handle,RawRxBuffer,RxBytes,&BytesReceived);
		if (FT_status == FT_OK) {

			if(parse)
			{
				device_and_success = parserx();
			}
			else
			{
				strcpy(ParsedRxBuffer, RawRxBuffer);
				device_and_success = 1;
			}

			clearConsole();
			
			if(printOrNot)
			{
				setTextGreen();
				printf("%s\n", ParsedRxBuffer);
				clearConsole();
				clearBuffers();
			}
	
			if(device_and_success > 0){return 1;}

		}
		else {
			printf("RX FAILED \n");
			clearConsole();
			return 0;
		}
	}
	clearConsole();
	return 0;
}


uint8_t parserx()
{
		// Parses data from LPC and HM-10.
		int successful = 0;
		
		// Does the RawRxBuffer contain an CR LF string?
		uint8_t *CR_LF_CHK = strstr(RawRxBuffer, "\r\n");
		uint8_t *LF_CHK = strstr(RawRxBuffer, "\n");

		uint8_t command_response_bfr[2];

		// Clear ParsedRxBuffer.
		for (int i = 0; i != sizeof(ParsedRxBuffer); ++i)
		{
			ParsedRxBuffer[i] = 0;
		}

		// If it does contain CR LF
		if(CR_LF_CHK != '\0')
		{
			// Load the Raw RX into the Parsed Rx.
			strcpy(ParsedRxBuffer, RawRxBuffer);
			
			// If the LPC responds, we want to remove CR and LF.
			// Let's replace CR LF with "  ".
			int replaceIndex = 0;
			while(replaceIndex < sizeof(ParsedRxBuffer))
			{
				if(ParsedRxBuffer[replaceIndex] == '\r' && ParsedRxBuffer[replaceIndex+1] == '\n')
				{
					// REWRITE TO SHORTEN STRING, INSTEAD OF ' '.
					ParsedRxBuffer[replaceIndex] = ' ';
					ParsedRxBuffer[replaceIndex+1] = ' ';
				}								
				replaceIndex++;
			}
			// About to reuse counter.
			replaceIndex=0;

			// Encodes the LPC reponse to be read by command_response().
			command_response_bfr[0] = ParsedRxBuffer[0];
			command_response_bfr[1] = ParsedRxBuffer[1];
			command_response_code = atoi(command_response_bfr);
			successful = 1;
		}

		// Is it an HM-10?
		else if (RawRxBuffer[0] == 'O' && RawRxBuffer[1] == 'K' && RawRxBuffer[2] == '+')
		{
			strcpy(ParsedRxBuffer, RawRxBuffer);
			// HM-10 responded.
			successful=2;
		}
		// If the RawRxBuffer data does not contain "\r\n" or "OK+" strings.
		else
		{
			strcpy(ParsedRxBuffer, RawRxBuffer);
			successful=0;
		}

		// Clear RawRxBuffer
		for (int i = 0; i != sizeof(RawRxBuffer); ++i)
		{
			RawRxBuffer[i] = 0x00;
		}
		return successful;
}

void clearBuffers()
{
	// Clear ParsedRxBuffer.
	for (int i = 0; i != sizeof(ParsedRxBuffer); ++i)
	{
		ParsedRxBuffer[i] = 0;
	}

		// Clear RawRxBuffer
	for (int i = 0; i != sizeof(RawRxBuffer); ++i)
	{
		RawRxBuffer[i] = 0;
	}
}


uint8_t txString(uint8_t string[], int txString_size, bool printOrNot, int frequency_of_tx_char)
{

	uint8_t FTWrite_Check;

	for (int i = 0; i < (txString_size-1); i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FTWrite_Check = FT_Write(handle, &string[i], (DWORD)sizeof(string[i]), &bytes);
		Sleep(frequency_of_tx_char);
		if(printOrNot)
		{
			setTextRed();
			printf("%C", string[i]);
		}
	}	
	clearConsole();
}

///////////// Debugging //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void writeUUEDataTofile(uint8_t UUE_Encoded_String[], int UUE_Encoded_String_Index)
{
	uint8_t UnixFilePermissions[] = "0777";

	UUEDataFile = open_file ("uueFile.uue", "w" );
	
	uint8_t UUELineCountCharacter;
	int total_char_index = 0;
	

	// Create UUE header 
	fprintf(UUEDataFile, "begin %s %s\n", UnixFilePermissions, "uueFile.uue");
	if (UUEDataFile == NULL) {
		printf("I couldn't open uueFile.uue for writing.\n");
		exit(0);
	}
	else
	{
		// Loop for total characters.
		for (int characterIndex = 0; characterIndex < UUE_Encoded_String_Index; characterIndex)
		{					
			// Loop for characters per line.			
			for (int lineIndex = 0; lineIndex < 61; ++lineIndex)
			{
					fprintf(UUEDataFile, "%c", UUE_Encoded_String[characterIndex]);	
					characterIndex++;
			}
			total_char_index++;
			fprintf(UUEDataFile, "\n");
		}
		fprintf(UUEDataFile, "'\n");	
		fprintf(UUEDataFile, "end\n");	
	}
}

void writeHexDataTofile(uint8_t * fileData_Hex_String, int hexDataCharCount)
{
	int totalDataIndex = 0;
	hexDataFile = open_file ("hexFile.hex", "w" );
	if (hexDataFile == NULL) {
		printf("I couldn't open hexFile.hex for writing.\n");
		exit(0);
	}
	else{
		for (int char_count_index = 0; totalDataIndex < hexDataCharCount; char_count_index)
		{
			for (int line_index = 0; line_index < 16; ++line_index)
			{
				fprintf(hexDataFile, "%02X", fileData_Hex_String[totalDataIndex]);
				totalDataIndex++;
				// If we reach the end-of-data, exit loops.
				if (totalDataIndex == hexDataCharCount){break;}
			}
			fprintf(hexDataFile, "\n");		
		}
		
	}
	
}


////////////////////////////// Data Handling (Conversion, etc.) //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//Copied in from lpc21isp.c
static uint8_t Ascii2Hex(uint8_t c)
{
	if (c >= '0' && c <= '9')
	{
		return (uint8_t)(c - '0');
	}
	if (c >= 'A' && c <= 'F')
	{
		return (uint8_t)(c - 'A' + 10);
	}
	if (c >= 'a' && c <= 'f')
	{
        return (uint8_t)(c - 'A' + 10);
	}
	//printf("\n !!! Bad Character: %02X in file at totalCharsRead=%d !!!\n\n", c, totalCharsRead);
	return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
} 



void clearSpecChar()
{
	//Removes CR, LF, ':'  --Bdk6's
	while (charToPut == '\n' || charToPut == '\r' || charToPut ==':'){
		(charToPut = fgetc (fileIn));
		totalCharsRead++;
	}
}


uint8_t readByte(){
	//Holds combined nibbles.
	uint8_t ASCII_hexvalue[3];
	uint8_t hex_hexvalue;
	char * pEnd;

	//Put first nibble in.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	ASCII_hexvalue[0] = (uint8_t)charToPut;
	
	//Put second nibble in.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	ASCII_hexvalue[1] = (uint8_t)charToPut;

	// Increase counter for total characters read from file.
	totalCharsRead+=2;

	// Convert the hex string to base 16.
	hex_hexvalue = strtol(ASCII_hexvalue, &pEnd, 16);
	
	return hex_hexvalue;	
}


int hex_file_to_array(uint8_t * hex_data_array, int file_size)
{
	//To hold file hex values.
	//uint8_t hex_data_array[MAX_SIZE];
	int hex_data_array_index = 0;
	

	uint8_t fhex_byte_count[MAX_SIZE_16];
	uint8_t fhex_address1[MAX_SIZE_16];
	uint8_t fhex_address2[MAX_SIZE_16];
	int combined_address = 0;
	uint8_t fhex_record_type[MAX_SIZE_16];
	uint8_t fhex_check_sum[MAX_SIZE_16];

	//To count through all characters in the file.
	int file_char_index = 0;
	int hex_data_index = 0;
	int hex_line_index = 0;
	
	//Holds line count.	
	int chars_this_line = 0;

	//Loop through each character until EOF.
	while(totalCharsRead  < file_size){
		
		//BYTE COUNT
		fhex_byte_count[file_char_index] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		fhex_address1[file_char_index] = readByte();
		
		//ADDRESS2
		fhex_address2[file_char_index] = readByte();
		
		//RECORD TYPE
		fhex_record_type[file_char_index] = readByte();	

		if (fhex_record_type[file_char_index] == 0)
		{
			combined_address = fhex_address1[file_char_index];
			combined_address <<= 8;
			combined_address |= fhex_address2[file_char_index];
			combined_address =  combined_address/16;
		}
		
		//Throws the byte count (data bytes in this line) into an integer.
		chars_this_line = fhex_byte_count[file_char_index];

		//////// DATA ///////////////////
		// We only want data. Discard other data types.
		if (fhex_record_type[file_char_index] == 0)
		{
			while (hex_data_index < chars_this_line && totalCharsRead < file_size && chars_this_line != 0x00)
			{
				//Store the completed hex value in the char array.
				hex_data_array[hex_data_array_index] = readByte();
				
				//Index for data.
				hex_data_array_index++;	
				hex_data_index++;			
			}

		//Reset loop index for characters on this line.
		hex_data_index = 0;
		}
		
		//////// CHECK SUM //////////////
		if (charToPut != 0xFF){
			fhex_check_sum[file_char_index] = readByte();
		}

		hex_line_index++;
		file_char_index++;
	}

	return hex_data_array_index;
}


int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size)
{
	// 1. Add char for characters per line.
	// 2. Load 3 bytes into an array.
	// 3. Encode array.
	// 4. Add padding.
	// 5. Replace ' ' with '''
	// 6. Return UUE data array (implicit) and size.
	uint8_t byte_to_encode[3];
	uint8_t uue_char[4];

	int UUE_encoded_string_index = 0;
	int uue_length_char_index = 45;
	int padded_index = 0;
	int bytes_left = 0;
	

	// 1. Add char for characters per line.
	if(hex_data_array_size < 45)
	{
		 UUE_data_array[UUE_encoded_string_index] = ((hex_data_array_size << 2) >> 2) + ' ';
	}
	else
	{
		UUE_data_array[UUE_encoded_string_index] = 'M';

	}
	printf("%C ", UUE_data_array[UUE_encoded_string_index]);
	UUE_encoded_string_index++;

	
	// Encode loop.
	for (int hex_data_array_index = 0; hex_data_array_index < hex_data_array_size; hex_data_array_index)
	{
		// 2. Load 3 bytes into an array.
		for (int i = 0; i < 3; ++i)
		{
			// Load bytes into array
			if (hex_data_array_index < hex_data_array_size)
			{
				byte_to_encode[i] = hex_data_array[hex_data_array_index];
				hex_data_array_index++;
			}
			else
			{
				// 4. Add padding.
				byte_to_encode[i] = 0;
				padded_index++;
			}
			uue_length_char_index--;
		}
		

		// 3. Encode array.
		uue_char[0] = ((byte_to_encode[0] >> 2) & 0x3f);
		uue_char[1] = (((byte_to_encode[0] << 4) | ((byte_to_encode[1] >> 4) & 0x0f)) & 0x3f);
		uue_char[2] = (((byte_to_encode[1] << 2) | ((byte_to_encode[2] >> 6) & 0x03)) & 0x3f);
		uue_char[3] = (byte_to_encode[2] & 0x3f);

		for (int i = 0; i < 4; i++)
		{
			// 5. Replace ' ' with '''
			if (uue_char[i] == 0x00)
			{
				UUE_data_array[UUE_encoded_string_index] = 0x60;
			}
			else
			{
				UUE_data_array[UUE_encoded_string_index] = (uue_char[i] + ' ');
			}
			printf("%C ", UUE_data_array[UUE_encoded_string_index]);

			UUE_encoded_string_index++;
		}

		// Data bytes left.
		bytes_left = (hex_data_array_size + (hex_data_array_index*-1));
		 
		if (uue_length_char_index == 0)
		{
			// NOTE: Could be simplified to include first char
			// and additional characters, using a positive index.
			// 1. Add char for characters per line.
			UUE_data_array[UUE_encoded_string_index] = '\n';
			UUE_encoded_string_index++;
			printf("bytes_left %i\n", bytes_left);
			if(bytes_left < 45)
			{
				printf("HERE1 %i   %i \n", UUE_encoded_string_index, uue_length_char_index);
				// Find how many characters are left.
				UUE_data_array[UUE_encoded_string_index] = ((bytes_left & 0x3f) + ' ');
			}
			else
			{
				printf("HERE2 \n");
				UUE_data_array[UUE_encoded_string_index] = 'M';
				printf("%C ", UUE_data_array[UUE_encoded_string_index]);
			
			}	
			UUE_encoded_string_index++;
			uue_length_char_index = 45;
		}

	} // End UUE loop	

	// Return count of UUE chars.
	return UUE_encoded_string_index;
}


void setTextRed()
{
	setColor(LIGHTRED, BLACK);
}
void setTextGreen()
{
	setColor(LIGHTGREEN, BLACK);
}

void setColor(int ForgC, int BackC)
{
     WORD wColor = ((BackC & 0x0F) << 4) + (ForgC & 0x0F);
     SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);
     return;
}

void clearConsole()
{
	setColor(WHITE, BLACK);
}

void startScreen()
{
	printf("\n");
	printf("\n");
	printf("**************************************************************************\n");
	printf("**** Mutant LPC1114 Downloader v.76                                  *****\n");
	printf("**** Hacked Out with Little Thought                                  *****\n");
	printf("****                                   Thanks to Bdk6                *****\n");
	printf("****                            His help was more than invaluable,   *****\n");
	printf("****                                 it was a necessity.             *****\n");
	printf("**************************************************************************\n");
	printf("\n");
	printf("\n");
	clearConsole();
}


void copy_string(uint8_t *target, uint8_t *source)
{
   while(*source)
   {
      *target = *source;
      source++;
      target++;
   }
   *target = '\0';
}

void command_response()
{
	// Pulls from global command_reponse_code, which is
	// set in parseRx().
	switch(command_response_code)
	{
		// 1
		case RESP_CMD_SUCCESS:
			printf("Command Success");
			break;
		// 2
		case RESP_INVALID_COMMAND:
			printf("Invalid Command");
			break;
		// 3
		case RESP_SRC_ADDR_ERROR:
			printf("Source Address Error");
			break;
		// 4
		case RESP_SRC_ADDR_NOT_MAPPED:
			printf("Source Address Not Mapped");
			break;
		// 5
		case RESP_DST_ADDR_NOT_MAPPED:
			printf("Destination Address Not Mapped");
			break;
		// 6
		case RESP_COUNT_ERROR:
			printf("Count Error");
			break;
		// 7
		case RESP_INVALID_SECTOR:
			printf("Invalid Sector");
			break;
		// 8
		case RESP_SECTOR_NOT_BLANK:
			printf("Sector Not Blank");
			break;
		// 9
		case RESP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION:
			printf("Sector Not Prepared for Write Operation");
			break;
		// 10
		case RESP_COMPARE_ERROR:
			printf("Compare Error");
			break;
		// 11
		case RESP_BUSY:
			printf("Busy");
			break;
		// 12
		case RESP_PARAM_ERROR:
			printf("Parameter Error");
			break;
		// 13
		case RESP_ADDR_ERROR:
			printf("Address Error");
			break;
		// 14
		case RESP_ADDR_NOT_MAPPED:
			printf("Address Not Mapped");
			break;
		// 15
		case RESP_CMD_LOCKED:
			printf("Command Locked");
			break;
		// 16
		case RESP_INVALID_CODE:
			printf("Invalid Code");
			break;
		// 17
		case RESP_INVALID_BAUD_RATE:
			printf("Invalid Baud Rate");
			break;
		// 18
		case RESP_INVALID_STOP_BIT:
			printf("Invalid Stop Bit");
			break;
		// 19
		case RESP_CODE_READ_PROTECTION_ENABLED:
			printf("Code Read Protection Enabled");
			break;			
	}
	// Clear for next use.
	command_response_code = 0;
}