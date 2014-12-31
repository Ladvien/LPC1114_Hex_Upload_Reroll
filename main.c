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

/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
A. Preparing the data.
1. Hex file accessed. (DONE) I noticed the FTDI chip needs to be using XON/XOFF, I assume I can use FTDI’s library to enable this functionality?
2. Hex file converted to string. Need to store hex data indexed by address.  And, I need to create a checksum to coordinate with the UUE string.  
As I understand it, the checksum is used by the LPC boot loader to ensure the UUE data uploaded is correct.  From what I read, the checksum is 
generated by adding the data bytes together.  Data is not delivered to the LPC in greater than 45 bytes at a time.  Therefore, the checksum 
should be the sum of every 45 bytes?
3. Hex string encoded into a UUE string. I need to divide UUE data into strings of 61 characters, which gives 45 bytes of data.
 
B. LPC-HOST Handshake
1. HOST Send: ‘?’
2. LPC auto-baud measures the data.  The host UART should be set to 8, 1, N.
3. LPC Responds: “Synchronized<CR><LF>”
4. Host should respond: “Synchronized<CR><LF>”
5. LPC Looks at the characters to verify synchronization.
6. LPC Responds: “OK<CR><LF>”
7. Host send crystal frequency in kHz: “12500<CR><LF>”
8. LPC Responds: “OK<CR><LF>”
 
C. Host Writes to LPC
1. Host sends Unlock command: “U 23130<CR><LF>”
2.  Host sends: “W <RAM Address in Decimal> 61<CR><LF>”
3. Host sends UUE line to RAM. Host sends: 61 characters of UUE
4. Host sends respective checksum.
5. If checksum matches, LPC responds: “OK<CR><LF>”
6. If checksum doesn’t match, LPC responds: “RESEND<CR><LF>.” The host should repeat steps 11 & 12.
7. ....we fill up so much of the RAM....
8. Host sends: “P 0 0<CR><LF>.”  The prepares the Flash sector 0 for writing.
9. Host sends: “C <RAM Address in Decimal> 512 0<CR><LF>.”  This should copy two pages of RAM data to Flash at sector 0.  
The smallest amount of data we may write to Flash is 256 bytes (one page).
10. Repeat 1-9 until all data is written to the LPC’s Flash memory.

*////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////// Forward Declaration ////////////////////////////////////////////////////////////

// Output files, for debugging purposes.
void writeUUEDataTofile(unsigned char UUE_Encoded_String[], int hexDataCharCount);
void writeHexDataTofile(unsigned char fileData_Hex_String[], int hexDataCharCount, unsigned char byteCount[], int hexFileLineCount);

// FTDI
static FILE *open_file ( char *file, char *mode );
int fileSizer();
unsigned char rx();
unsigned char parseRX();
char txString(char string[], int txString_size);
int FTDI_State_Machine(int state, int FT_Attempts);

// LPC handling
unsigned char set_ISP_mode();

// Data Handling
struct hexFile hexFileToCharArray();
int readByte();
void clearSpecChar();
int Hex2Ascii(unsigned char hexValue);
int Hex2Int(unsigned char c);
static unsigned char Ascii2Hex(unsigned char c);
struct UUE_Data UUencode();

//////////////////// Variables and Defines ////////////////////////////////////////////////////////

//#define PIN_TX  0x01  /* Orange wire on FTDI cable */
//#define PIX_RX  0x02  /* Yellow */
//#define PIN_RTS 0x04  /* Green */
#define PIN_CTS 0x08  /* Brown */
#define PIN_DTR 0x10
//#define PIN_DSR 0x20
//#define PIN_DCD 0x40
//#define PIN_RI  0x80

	// HM-11-A sends "AT+PIO30"
	// The HM-11-B PIO3 will go low, which sets the LPC1114 ISP MODE to LOW.  
	// HM-11-A sends "AT+PIO20"
	// Then, the HM-11-B PIO2 goes LOW for ~5uS.  This will reset the LPC.
	// As the LPC comes back up it checks the ISP MODE pin and sees it LOW.
	// HM-11-A sends "AT+PIO31" waits ~100mS then sends "AT+PIO21"
	// The HM-11-B PIO3 and PIO2 will go HIGH.
	// The LPC enters ISP mode.
	// The HM-11 PIO2 & PIO3 go HIGH.
	// The program is uploaded to the LPC.
	// HM-11-A sends "AT+PIO20"
	// HM-11-B PIO2 goes LOW, resetting the LPC.
	// HM-11-A sends "AT+PIO21"
	// HM-11-B PIO2 goes HIGH and the LPC runs the uploaded program.

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

//Serial Port handle.
//Used by FTD2XX
FT_HANDLE handle = NULL;
FT_STATUS FT_status;
DWORD EventDWord;
DWORD TxBytes;
DWORD bytes;
DWORD RxBytes;
DWORD BytesReceived;
unsigned char RxBuffer[256];

//File to be loaded.	
FILE *fileIn;
FILE *hexDataFile;
FILE *UUEDataFile;

//Reading characters from a file.
unsigned char charToPut;

//Total bytesRead.
int totalCharsRead = 0;

struct hexFile {
	//To hold file hex values.
	unsigned char fileData_Hex_String[MAX_SIZE];
	int fileData_Hex_String_Size; unsigned char fhexByteCount[MAX_SIZE_16];
	int hexFileLineCount;
	unsigned char fhexAddress1[MAX_SIZE_16];
	unsigned char fhexAddress2[MAX_SIZE_16];
	unsigned char fhexRecordType[MAX_SIZE_16];
	unsigned char fhexCheckSum[MAX_SIZE_16];

};

struct UUE_Data{
	unsigned char UUE_Encoded_String[MAX_SIZE];
	unsigned char b[3];
	unsigned char d[4];

	int paddedIndex;
	int UUE_Encoded_String_Index;
};


// States for FTDI state machine.
typedef enum {
    FTDI_SM_OPEN,
   	FTDI_SM_RESET,
    FTDI_SM_CLOSE,
    FTDI_SM_ERROR,
} FTDI_state;

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
	// Stores hexFile data.
	struct hexFile hexFile;

	// Stores UUE data.
	struct UUE_Data UUE_Data;

	//For setting state of DTR/CTS.
	unsigned char DTR_Switch;
	unsigned char CTS_Switch;

	unsigned char rxString[256];

	// Stores file size.
	int fileSize;

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
	UUE_Data = UUencode();

	writeUUEDataTofile(UUE_Data.UUE_Encoded_String, UUE_Data.UUE_Encoded_String_Index);
	
	// Set LPC into ISP mode.
	for (int i = 0; i < FT_ATTEMPTS; ++i)
	{
		int returnVal = 0;
		returnVal = set_ISP_mode();
		printf("RETURN VAL: %i\n", returnVal);
		if (returnVal)
		{
			break;
		}
		else
		{
			printf("Can't contact LPC\n");
		}
		
	}
	
	/*
	//Sleep(500);
	//txString(HM_RESET, sizeof(HM_RESET));
	
	// Set crystal
	txString("12000\n", sizeof("12000\n"));
	Sleep(500);
	rx(5000, rxString);

	// Send Unlock Code
	txString("U 23130\n", sizeof("U 23130\n"));
	Sleep(500);
	rx(5000, rxString);



	// Read memory
	txString("R 268436224 4\n", sizeof("R 268436224 4\n"));
	Sleep(500);
	rx(5000, rxString);
	txString("OK\n", sizeof("OK\n"));
	Sleep(500);
	rx(5000, rxString);

	


		// Write memory
		txString("W 268436224 8\n", sizeof("W 268436224 8\n"));
		Sleep(500);
		rx(5000, rxString);

		txString("(5&AE(&-A<@\n403\n", sizeof("(5&AE(&-A<@\n403\n"));
		
		// TEST
		// HEX: 54 68 65 20 63 61 72
		// DECL 84  104 101 32 99 97
		// UUE: 5&AE(&-A

		printf("THIS NUMBER: %i\n", "RESEND");

		Sleep(500);
		rx(5000, rxString);

	// Read memory
	txString("R 268436224 4\n", sizeof("R 268436224 4\n"));
	Sleep(500);
	rx(5000, rxString);
	txString("OK\n", sizeof("OK\n"));
	Sleep(500);
	rx(5000, rxString);

	//txString("#0V%T\n", sizeof("#0V%T\n"));
	//Sleep(500);
	//rx(5000, rxString);
	*/

	//Close files.
	fclose ( fileIn );
	fclose ( UUEDataFile );
	fclose ( hexDataFile );

} // END PROGRAM


///////////// LPC Handling ///////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

unsigned char set_ISP_mode()
{
	unsigned char rxString[256];

	txString(HM_ISP_LOW, sizeof(HM_ISP_LOW));
	Sleep(500);
	rx(1);

	txString(HM_LPC_RESET_LOW, sizeof(HM_LPC_RESET_LOW));
	Sleep(500);
	rx(1);

	txString(HM_LPC_RESET_HIGH, sizeof(HM_LPC_RESET_HIGH));
	Sleep(500);
	rx(1);

	txString(HM_ISP_HIGH, sizeof(HM_ISP_HIGH));
	Sleep(500);
	rx(1);

	// Synchronized check.
	txString(LPC_CHECK, sizeof(LPC_CHECK));
	Sleep(500);
	rx(1);

	txString(Synchronized, sizeof(Synchronized));
	Sleep(500);
	rx(1);

	// CHECK IF IN RESET MODE

	return 1;
}

unsigned char get_LPC_Info()
{
	unsigned char rxString[256];

	// Read Part ID
	txString("J\n", sizeof("J\n"));
	Sleep(500);
	printf("Read Part ID:\n");
	rx(1);

	// Boot Version
	txString("K\n", sizeof("K\n"));
	Sleep(500);
	printf("Read Boot Version:\n");
	rx(1);

	// Read Part ID
	txString("N\n", sizeof("N\n"));
	Sleep(500);
	printf("Read UID:\n");
	rx(1);

	// ADD PARSING AND PRINTING
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


unsigned char rx(int device)
{
	//FT_SetTimeouts(handle, timeout, 0);
	FT_GetStatus(handle, &RxBytes, &TxBytes, &EventDWord);
	if (RxBytes > 0) {
		FT_status = FT_Read(handle,RxBuffer,RxBytes,&BytesReceived);
		if (FT_status == FT_OK) {
			while(parseRX()!=true);			
		}
		else {
			printf("RX FAILED \n");
		}
	}
}


unsigned char parseRX()
{
		// GET DATA
		
		// Print data.
		//printf("RX: %s\n", RxBuffer);

		bool successful = false;

		// Does the RxBuffer contain an CR LF string?
		unsigned char *CR_LF_CHK = strstr(RxBuffer, "\r\n");
		// If it does contain CR LF
		if(CR_LF_CHK != '\0')
		{
				// Let's replace CR LF with "  ".
				int replaceIndex = 0;
				printf("RX: ");
				while(RxBuffer[replaceIndex] != 0x00)
					{
						if(RxBuffer[replaceIndex] == '\r' || RxBuffer[replaceIndex] == '\n'){successful = true;;}
						else{putchar(RxBuffer[replaceIndex]);}									
						replaceIndex++;
					}
				// Silly, but let's add our own LF.
				printf("\n");
		}
		
		// If the RX'ed data does not contain CR LF string.
		else
		{
			// Let's print what's in the buffer and add a LF.
			printf("RX: %s\n", RxBuffer);
		}

		if (RxBuffer[0] == 'O' && RxBuffer[1] == 'K' && RxBuffer[2] == '+')
		{
			successful=true;
			printf("LPC\n");
		}

		// Clear RxBuffer
		for (int i = 0; i != sizeof(RxBuffer); ++i)
		{
			RxBuffer[i] = 0;
		}
		return successful;
}


/*
unsigned char * rx(int timeout, unsigned char *rxString)
{
	// Less than ~40ms and characters get garbled?
	Sleep(80);
	//FT_SetTimeouts(handle, timeout, 0);
	FT_GetQueueStatus(handle,&RxBytes);
	//FT_GetStatus(handle, &RxBytes, &TxBytes, &EventDWord);
	//printf("RX: %s  RxBytes: %i         BytesReceived:   %i\n",RxBuffer, RxBytes, BytesReceived);
	if (RxBytes > 0) {
		FT_status = FT_Read(handle,RxBuffer,RxBytes,&BytesReceived);
		if (FT_status == FT_OK) {
			
			// GET DATA
			
			//FT_status = FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX); // Purge both Rx and Tx buffers

// Removes CR and LF for debugging.
/*
			for (int i = 0; i != sizeof(RxBuffer); ++i)
			{
				if (RxBuffer[i] == '\r')
				{
					RxBuffer[i] = 'R';
				}
				if (RxBuffer[i] == '\n')
				{
					RxBuffer[i] = 'N';
				}
			}


			// Print data.
			//printf("RX: %s ------------ RxBytes: %i BytesReceived: %i\n",RxBuffer, RxBytes, BytesReceived);
			printf("RX: %s\n", RxBuffer);
			
			for (int i = 0; i != sizeof(RxBuffer); ++i)
			{
				RxBuffer[i] = 0;
			}
		}
		else {
			printf("RX FAILED \n");
		}
		//printf("\n");
	}
}

*/

char txString(char string[], int txString_size)
{
	unsigned char FTWrite_Check;
	for (int i = 0; i < (txString_size-1); i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FTWrite_Check = FT_Write(handle, &string[i], (DWORD)sizeof(string[i]), &bytes);
	}	

	// Let's check if the send string contains a newline character.
	unsigned char * newLineTest = strstr(string, "\n");
	// No? Let's add it.
	if (newLineTest == '\0')
	{
		printf("TX: %s\n", string);
	}
	else{
		printf("TX: %s", string);	
	}
	
}

///////////// Debugging //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void writeUUEDataTofile(unsigned char UUE_Encoded_String[], int UUE_Encoded_String_Index)
{
	unsigned char UnixFilePermissions[] = "0777";

	UUEDataFile = open_file ("uueFile.uue", "w" );
	
	unsigned char UUELineCountCharacter;
	int test = 0;

	// 764 hex characters.
	// 1018.66666 UUE characters


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
			if (test == 0)
			{
				// Add line count character at beginning of line.
				fprintf(UUEDataFile, "M");		
			}
			else
			{
				// Add line count character at beginning of line.
				UUELineCountCharacter = characterIndex/test;
				UUELineCountCharacter = (UUELineCountCharacter + ' ');
				fprintf(UUEDataFile, "%c", UUELineCountCharacter);		
			}
		
			// Loop for characters per line.			
			for (int lineIndex = 0; lineIndex < 45; ++lineIndex)
			{
				if (UUE_Encoded_String[characterIndex] == ' ')
				{
					fprintf(UUEDataFile, "'");
					characterIndex++;
				}
				else
				{
					fprintf(UUEDataFile, "%c", UUE_Encoded_String[characterIndex]);	
					characterIndex++;
				}
			}
			test++;
			fprintf(UUEDataFile, "\n");
			
			//printf("\n\n\n%i", UUE_Encoded_String_Index);
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

	//Loop through each character until EOF.
	while(totalCharsRead  < fileSize){
		
		//BYTE COUNT
		hexFile.fhexByteCount[i] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		hexFile.fhexAddress1[i] = readByte();
		
		//ADDRESS2
		hexFile.fhexAddress2[i] = readByte();
		
		// Tutorial for converting 16 bits to int.
		// http://homepage.cs.uiowa.edu/~jones/bcd/decimal.html
		//printf("%02X %02X\n", hexFile.fhexAddress1[i], hexFile.fhexAddress2[i]);
		//printf("A: %i  B: %i\n", hexFile.fhexAddress1[i], hexFile.fhexAddress2[i]);
		//printf("%hu\n", fullAddress);
		//addressCombine();
		
		//RECORD TYPE
		hexFile.fhexRecordType[i] = readByte();	

		if (hexFile.fhexRecordType[i] == 0)
		{
			fullAddress = hexFile.fhexAddress1[i];
			fullAddress <<= 8;
			fullAddress |= hexFile.fhexAddress2[i];
			fullAddress =  fullAddress/16;
			printf("%i\n", fullAddress);
		}
		
		//printf("%02X %02X\n", hexFile.fhexAddress1[i], hexFile.fhexAddress2[i]);
		//printf("A: %i  B: %i\n", hexFile.fhexAddress1[i], hexFile.fhexAddress2[i]);
		

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
				
				//Index for data.
				hexFile.fileData_Hex_String_Size++;
				//Index for loop.
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
	return hexFile;
}

struct UUE_Data UUencode()
{
	// ASCII->HEX->UUE Test Strings
	// ASCII: The witch Lilith knows my soul.
	// HEX: {0x54, 0x68, 0x65, 0x20, 0x77, 0x69, 0x74, 0x63, 0x68, 0x20, 0x4c, 0x69, 0x6c, 0x69, 0x74 0x68, 0x20, 0x6b, 0x6e, 0x6f, 0x77, 0x73, 0x20, 0x6d, 0x79, 0x20, 0x73, 0x6f, 0x75, 0x6c, 0x2e};
	// UUE-ASCII: ?5&AE('=I=&-H($QI;&ET:"!K;F]W<R!M>2!S;W5L+@
	// UUE-HEX:{35 26 41 45 28 27 3d 49 3d 26 2d 48 28 24 51 49 3b 26 45 54 3a 22 21 4b 3b 46 5d 57 3c 52 21 4d 3e 32 21 53 3b 57 35 4c 2b 40}
	//unsigned char testUUE_HEX[] = {0x54, 0x68, 0x65, 0x20, 0x63, 0x61, 0x72, 0x00};

	// Stores hexFile data.
	struct hexFile hexFile;

	struct UUE_Data UUE_Data;
	//unsigned char fileData_Hex_String[], int hexDataCharCount

	for(int hexDataIndex = 0;  hexDataIndex < hexFile.fileData_Hex_String_Size; hexDataIndex)
	{
		// Load chars or nulls
		for (int i = 0; i < 3; i++)
		{

			if (hexDataIndex < hexFile.fileData_Hex_String_Size)
			{
				UUE_Data.b[i] = hexFile.fileData_Hex_String[hexDataIndex];	
			}
			else
			{
				UUE_Data.b[i] = 0;
				UUE_Data.paddedIndex++;
			}
			hexDataIndex++;
		}
		
		// UUEncode
		UUE_Data.d[0] = (((UUE_Data.b[0] >> 2) & 0x3f) + ' ');
		UUE_Data.d[1] = ((((UUE_Data.b[0] << 4) | ((UUE_Data.b[1] >> 4) & 0x0f)) & 0x3f) + ' ');
		UUE_Data.d[2] = ((((UUE_Data.b[1] << 2) | ((UUE_Data.b[2] >> 6) & 0x03)) & 0x3f) + ' ');
		UUE_Data.d[3] = ((UUE_Data.b[2] & 0x3f) + ' ');
		
		// Put the UUEncoded chars into their own string.
		for (int i = 0; i < 4; i++)
		{
			UUE_Data.UUE_Encoded_String[UUE_Data.UUE_Encoded_String_Index] = UUE_Data.d[i];
			UUE_Data.UUE_Encoded_String_Index++;
		}
	}
	
	// ADD FOR-LOOP TO DIVIDE UUE DATA INTO 61 CHAR STRINGS.

	/*
	for (int i = 0; i < (UUE_Encoded_String_Index-paddedIndex); ++i)
	{
		printf(" 0x%2x \n", UUE_Encoded_String[i]);
	}
	*/
	UUE_Data.UUE_Encoded_String_Index = UUE_Data.UUE_Encoded_String_Index - UUE_Data.paddedIndex;
	return UUE_Data;
}