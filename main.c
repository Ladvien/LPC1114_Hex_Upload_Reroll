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
#include "ftd2xx.h"

//////////////////// Forward Declaration ////////////////////////////////////////////////////////////

// Output files, for debugging purposes.
void writeUUEDataTofile(unsigned char UUE_Encoded_String[], int hexDataCharCount);
void writeHexDataTofile(unsigned char fileData_Hex_String[], int hexDataCharCount, unsigned char byteCount[], int hexFileLineCount);

// FTDI
static FILE *open_file ( char *file, char *mode );
int fileSizer();
unsigned char rx(bool parse, bool printOrNot);
unsigned char parserx();
char txString(char string[], int txString_size, bool printOrNot, int frequency_of_tx_char);
int FTDI_State_Machine(int state, int FT_Attempts);
unsigned char get_LPC_Info(bool print);

void command_response();

// LPC handling
unsigned char set_ISP_mode(int print);

// Data Handling
struct hexFile hexFileToCharArray();
int readByte();
void clearSpecChar();
int Hex2Ascii(unsigned char hexValue);
int Hex2Int(unsigned char c);
static unsigned char Ascii2Hex(unsigned char c);
struct UUE_Data UUencode(struct hexFile hexFile);
void clearBuffers();

void setTextRed();
void setTextGreen();
void setColor(int ForgC, int BackC);
void clearConsole();
void copy_string(char *target, char *source);
void startScreen();
void OK();
void Failed();
void check_HM_10();
void wake_devices();

struct writeToRam write_to_ram();

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
unsigned char RawRxBuffer[256];
unsigned char ParsedRxBuffer[256];

//File to be loaded.	
FILE *fileIn;
FILE *hexDataFile;
FILE *UUEDataFile;

//Reading characters from a file.
unsigned char charToPut;

//Total bytesRead.
int totalCharsRead = 0;

int command_response_code;

struct hexFile {
	//To hold file hex values.
	unsigned char fileData_Hex_String[MAX_SIZE];
	
	int fileData_Hex_String_Size; 
	int hexFileLineCount;
	
	long long int original_data_checksum[MAX_SIZE_16];

	unsigned char fhexByteCount[MAX_SIZE_16];
	unsigned char fhexAddress1[MAX_SIZE_16];
	unsigned char fhexAddress2[MAX_SIZE_16];
	unsigned char fhexRecordType[MAX_SIZE_16];
	unsigned char fhexCheckSum[MAX_SIZE_16];

};

struct UUE_Data{
	unsigned char UUE_Encoded_String[MAX_SIZE];
	unsigned char b[3];
	unsigned char d[4];
	int uue_length_char_index;
	int paddedIndex;
	int UUE_Encoded_String_Index;
};


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



int fullAddress = 0;

///////////////////// PROGRAM ////////////////////////////
////////////////////// START /////////////////////////////

int main(int argc, char *argv[])
{


	clearConsole();
	
	startScreen();	
	// Stores hexFile data.
	struct hexFile hexFile;

	// Stores UUE data.
	struct UUE_Data UUE_Data;

	struct writeToRam writeToRam;

	//For setting state of DTR/CTS.
	unsigned char DTR_Switch;
	unsigned char CTS_Switch;

	// Stores file size.
	int fileSize = 0;

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

	//Convert file to one long char array.
	hexFile = hexFileToCharArray(fileSize);

	// Write hex string back to a file.  Used for debugging.
	writeHexDataTofile(hexFile.fileData_Hex_String, hexFile.fileData_Hex_String_Size, hexFile.fhexByteCount, hexFile.hexFileLineCount);
	
	// Convert hex data to UUE.
	UUE_Data = UUencode(hexFile);

	// Write the UUE string to a file.  CURRENTLY BROKEN-ish.
	writeUUEDataTofile(UUE_Data.UUE_Encoded_String, UUE_Data.UUE_Encoded_String_Index);
	
	// Let's wake the device chain (FTDI, HM-10, HM-10, LPC)
	wake_devices();
	
	// Check the RSSI of HM-10.
	//check_HM_10();

	// Clear the console color.
	clearConsole();
	
	// Set LPC into ISP mode.
	set_ISP_mode(PRINT);

	// Get LPC Device info.
	//get_LPC_Info(PRINT);

	//Sleep(500); 
	//txString(HM_RESET, sizeof(HM_RESET), PRINT, 0);
	
	// Send Unlock Code
	txString("U 23130\n", sizeof("U 23130\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, NO_PRINT);

	// DEBUG NOTES:
	// It seems the hexFile.original_data_checksum printed from the hexRead function
	// does not equal what it does from the main function.
	char write_to_ram_string[64];
	// Turn UUE string size into ASCII.
	int tx_byte_count[64];
	tx_byte_count[0] = UUE_Data.UUE_Encoded_String_Index;
	// Turn hex data checksum into ASCII.
	char tx_check_sum[64];
	sprintf(tx_check_sum, "%i", (hexFile.original_data_checksum[0]));
	//printf("%i\n", hexFile.original_data_checksum);
	//printf("%s\n", tx_check_sum);
	printf("BYTE COUNT: %i\n", tx_byte_count[0]);

	sprintf(write_to_ram_string, "W 268436224 %i\n", tx_byte_count[0]);
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

	//write_to_ram();
	




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




struct writeToRam write_to_ram()
{
	// Stores hexFile data.
	struct hexFile hexFile;

	// Stores UUE data.
	struct UUE_Data UUE_Data;

	for (int i = 0; i < 1200; ++i)
	{
		printf("%02X ", UUE_Data.UUE_Encoded_String[i]);
	}
	

}

///////////// LPC Handling ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

unsigned char set_ISP_mode(int print)
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
	unsigned char char_RSSI[2];
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


unsigned char get_LPC_Info(bool print)
{
	int successful = 0;
	unsigned char PartID[256];
	unsigned char UID[256];
	unsigned char BootVersion[256];

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


unsigned char rx(bool parse, bool printOrNot)
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


unsigned char parserx()
{
		// Parses data from LPC and HM-10.
		int successful = 0;
		
		// Does the RawRxBuffer contain an CR LF string?
		unsigned char *CR_LF_CHK = strstr(RawRxBuffer, "\r\n");
		unsigned char *LF_CHK = strstr(RawRxBuffer, "\n");

		unsigned char command_response_bfr[2];

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


char txString(char string[], int txString_size, bool printOrNot, int frequency_of_tx_char)
{

	unsigned char FTWrite_Check;

	for (int i = 0; i < (txString_size-1); i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FTWrite_Check = FT_Write(handle, &string[i], (DWORD)sizeof(string[i]), &bytes);
		Sleep(frequency_of_tx_char);
	}	

	// Let's check if the send string contains a newline character.
	unsigned char * newLineTest = strstr(string, "\n");
	// No? Let's add it.
	if(printOrNot){
		if (newLineTest == '\0')
		{
			setTextRed();
			printf("%s\n", string);
		}
		else{
			setTextRed();
			printf("%s", string);	
		}	
	}
}

///////////// Debugging //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void writeUUEDataTofile(unsigned char UUE_Encoded_String[], int UUE_Encoded_String_Index)
{
	unsigned char UnixFilePermissions[] = "0777";

	UUEDataFile = open_file ("uueFile.uue", "w" );
	
	unsigned char UUELineCountCharacter;
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

void writeHexDataTofile(unsigned char fileData_Hex_String[], int hexDataCharCount, unsigned char byteCount[], int hexFileLineCount)
{
	
	hexDataFile = open_file ("hexFile.hex", "w" );
	if (hexDataFile == NULL) {
		printf("I couldn't open hexFile.hex for writing.\n");
		exit(0);
	}
	
	for (int totalDataIndex = 0; totalDataIndex < hexDataCharCount; totalDataIndex)
	{
		for (int lineIndex = 0; lineIndex < hexFileLineCount; ++lineIndex)
		{
			int bytesThisLine = (byteCount[lineIndex]);
			for (int i = 0; i < bytesThisLine && totalDataIndex < hexDataCharCount; ++i)  //need to change 16 to byteCount.
			{
				if ((fileData_Hex_String[totalDataIndex]) != 0x00)
				{
					fprintf(hexDataFile, "%02X", fileData_Hex_String[totalDataIndex]);
				}
				else
				{
					fprintf(hexDataFile, "00", fileData_Hex_String[totalDataIndex]);
				}
				totalDataIndex++;
			}
		fprintf(hexDataFile, "\n");
		}

	}
}


////////////////////////////// Data Handling (Conversion, etc.) //////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//Copied in from lpc21isp.c
static unsigned char Ascii2Hex(unsigned char c)
{
	if (c >= '0' && c <= '9')
	{
		return (unsigned char)(c - '0');
	}
	if (c >= 'A' && c <= 'F')
	{
		return (unsigned char)(c - 'A' + 10);
	}
	if (c >= 'a' && c <= 'f')
	{
        return (unsigned char)(c - 'A' + 10);
	}
	//printf("\n !!! Bad Character: 0x%2x in file at totalCharsRead=%d !!!\n\n", c, totalCharsRead);
	return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
} 

int Hex2Int(unsigned char c)
{
	int first = c / 16 - 3;
	int second = c % 16;
	int result = first*10 + second;
	//if(result > 9) result--;
	return result;
}

int Hex2Ascii(unsigned char hexValue)
{
	unsigned char c = hexValue >>= 4;
	hexValue <<= 4;
	unsigned char d = hexValue >>= 4;

	int high = Hex2Int(c) * 16;
	int low = Hex2Int(d);
	return high+low;
}

void clearSpecChar()
{
	//Removes CR, LF, ':'  --Bdk6's
	while (charToPut == '\n' || charToPut == '\r' || charToPut ==':'){
		(charToPut = fgetc (fileIn));
		totalCharsRead++;
	}
}

int readByte(){
	//Holds combined nibbles.
	unsigned char hexValue;
	
	//Get first nibble.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	//Put first nibble in.
	hexValue = (Ascii2Hex(charToPut));
	//Slide the nibble.
	hexValue <<= 4;
	//Put second nibble in.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	//Put the nibbles together.
	hexValue |= (Ascii2Hex(charToPut));
	//Return the byte.
	totalCharsRead=totalCharsRead+2;
	return hexValue;
}


//Convert file to one long char array.
struct hexFile hexFileToCharArray(int fileSize)
{
	struct hexFile hexFile;

	//To count through all characters in the file.
	int i = 0;
	int hexDataIndex = 0;

	//Holds line count.	
	int charsThisLine;

	int check_sum_count_45 = 0;
	int check_sum_index = 0;

	//Loop through each character until EOF.
	while(totalCharsRead  < fileSize){
		
		//BYTE COUNT
		hexFile.fhexByteCount[i] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		hexFile.fhexAddress1[i] = readByte();
		
		//ADDRESS2
		hexFile.fhexAddress2[i] = readByte();
		
		//RECORD TYPE
		hexFile.fhexRecordType[i] = readByte();	

		if (hexFile.fhexRecordType[i] == 0)
		{
			fullAddress = hexFile.fhexAddress1[i];
			fullAddress <<= 8;
			fullAddress |= hexFile.fhexAddress2[i];
			fullAddress =  fullAddress/16;
		}
		
		//Throws the byte count (data bytes in this line) into an integer.
		charsThisLine = hexFile.fhexByteCount[i];

		//////// DATA ///////////////////
		// We only want data.
		if (hexFile.fhexRecordType[i] == 0)
		{
			while (hexDataIndex != charsThisLine && totalCharsRead  < fileSize && charsThisLine != 0x00)
			{
				//Store the completed hex value in the char array.
				hexFile.fileData_Hex_String[hexFile.fileData_Hex_String_Size] = readByte();
				

				hexFile.original_data_checksum[check_sum_index] += hexFile.fileData_Hex_String[hexFile.fileData_Hex_String_Size];
				if(check_sum_count_45 > 900)
				{
					check_sum_index++;
					check_sum_count_45 = 0;
				}
				
				
				//Index for data.
				hexFile.fileData_Hex_String_Size++;
				//Index for loop.
				
				check_sum_count_45++;

				hexDataIndex++;
			}
		//Reset loop index.
		hexDataIndex=0;
		}

		
		//////// CHECK SUM //////////////
		if (charToPut != 0xFF){
			hexFile.fhexCheckSum[i] = readByte();
		}

		hexFile.hexFileLineCount++;
		i++;
	}
	//printf("%i\n", hexFile.original_data_checksum);
	return hexFile;
}

struct UUE_Data UUencode(struct hexFile hexFile)
{
	// ASCII->HEX->UUE Test Strings
	// ASCII: The witch Lilith knows my soul.
	// HEX: {0x54, 0x68, 0x65, 0x20, 0x77, 0x69, 0x74, 0x63, 0x68, 0x20, 0x4c, 0x69, 0x6c, 0x69, 0x74 0x68, 0x20, 0x6b, 0x6e, 0x6f, 0x77, 0x73, 0x20, 0x6d, 0x79, 0x20, 0x73, 0x6f, 0x75, 0x6c, 0x2e};
	// UUE-ASCII: ?5&AE('=I=&-H($QI;&ET:"!K;F]W<R!M>2!S;W5L+@
	// UUE-HEX:{35 26 41 45 28 27 3d 49 3d 26 2d 48 28 24 51 49 3b 26 45 54 3a 22 21 4b 3b 46 5d 57 3c 52 21 4d 3e 32 21 53 3b 57 35 4c 2b 40}

	// Stores UUE data.
	struct UUE_Data UUE_Data;
	
	// Set up characters per line index.
	UUE_Data.uue_length_char_index = 45;
	
	// Let's add the first char representing the lines per character
	// M = 45, $ = 4, etc.
	if (hexFile.fileData_Hex_String_Size < 45)
	{
		hexFile.fileData_Hex_String_Size = hexFile.fileData_Hex_String_Size + ' ';
	}
	else
	{
		UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = 'M';
	}
	UUE_Data.UUE_Encoded_String_Index++;		

	// Main UUE loop.
	for(int hexDataIndex = 0;  hexDataIndex < hexFile.fileData_Hex_String_Size; hexDataIndex)
	{

		// Load chars or nulls
		for (int i = 0; i < 3; i++)
		{
			// Load bytes in to array.
			if (hexDataIndex < hexFile.fileData_Hex_String_Size)
			{
				UUE_Data.b[i] = hexFile.fileData_Hex_String[hexDataIndex];	
			}
			else
			{
				// Padding with zeros.
				UUE_Data.b[i] = 0;
				UUE_Data.paddedIndex++;
			}
			hexDataIndex++;
			UUE_Data.uue_length_char_index--;
		}
		
		// UUEncode
		UUE_Data.d[0] = ((UUE_Data.b[0] >> 2) & 0x3f);
		UUE_Data.d[1] = (((UUE_Data.b[0] << 4) | ((UUE_Data.b[1] >> 4) & 0x0f)) & 0x3f);
		UUE_Data.d[2] = (((UUE_Data.b[1] << 2) | ((UUE_Data.b[2] >> 6) & 0x03)) & 0x3f);
		UUE_Data.d[3] = (UUE_Data.b[2] & 0x3f);
		
		// Replace 6-bit groups == 0x00
		// with 0x60.  Required by LPC.	

		// Put the UUEncoded chars into their own string.
		for (int i = 0; i < 4; i++)
		{
			if (UUE_Data.d[i] == 0x00)
			{
				printf("SPACE\n");
				UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] == 0x60;
			}
			else
			{
				UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = (UUE_Data.d[i] + ' ');
				UUE_Data.UUE_Encoded_String_Index++;
			}
			
			printf("%i\n", UUE_Data.UUE_Encoded_String_Index);
		}
		
		// Lets add data bytes per line character.
		if (UUE_Data.uue_length_char_index == 0)
		{
			// If the line is less than 45, let's calculate the char count.
			if (((hexDataIndex*-1) + hexFile.fileData_Hex_String_Size) < 45)
			{
				// Byte data index inverted plus the string size, converted to 6-bit ASCII.
				// This should only be for the last line.
				UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = 
				((hexDataIndex*-1)+hexFile.fileData_Hex_String_Size) + ' ';
				UUE_Data.UUE_Encoded_String_Index++;
			}
			else
			{
				// If it's a normal line (>45 bytes), add M, which is 6-bit ASCII for 45.
				UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = 'M';
				UUE_Data.UUE_Encoded_String_Index++;	
				UUE_Data.uue_length_char_index = 45;
			}
		}
	}

	// Let's set the UUE String Index (count) compsenating for null pads.
	UUE_Data.UUE_Encoded_String_Index = UUE_Data.UUE_Encoded_String_Index - UUE_Data.paddedIndex;

	// Let's make sure the string is divisible by 4.
	if(!(UUE_Data.UUE_Encoded_String_Index % 4 == 0))
	{
		UUE_Data.UUE_Encoded_String_Index++;
		UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = 0x00;
	}
	//for (int i = 0; i < UUE_Data.UUE_Encoded_String_Index; i)
	//{	
	//	printf("%C", UUE_Data.UUE_Encoded_String[i]);
	//	i++;		
	//}

	return UUE_Data;
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
	printf("**** Mutant LPC1114 Downloader v.1                                   *****\n");
	printf("**** Hacked Out with Little Thought                                  *****\n");
	printf("****                                   Thanks to Bdk6                *****\n");
	printf("****                            His help was more than invaluable,   *****\n");
	printf("****                                 it was a necessity.             *****\n");
	printf("**************************************************************************\n");
	printf("\n");
	printf("\n");
	clearConsole();
}


void copy_string(char *target, char *source)
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