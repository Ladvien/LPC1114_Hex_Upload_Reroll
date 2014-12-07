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
#include "ftd2xx.h"

//#define PIN_TX  0x01  /* Orange wire on FTDI cable */
//#define PIX_RX  0x02  /* Yellow */
//#define PIN_RTS 0x04  /* Green */
#define PIN_CTS 0x08  /* Brown */
#define PIN_DTR 0x10
//#define PIN_DSR 0x20
//#define PIN_DCD 0x40
//#define PIN_RI  0x80

#define MAX_SIZE 32768

//Serial Port handle.
FT_HANDLE handle;


//File to be loaded.	
FILE *fileIn;
int fileSize = 0;
char * fileCharPoint;

//Reading characters from a file.
unsigned char charToPut;

//Total bytesRead.
int totalCharsRead = 0;

struct hexFile {

	//To hold file hex values.
	unsigned char fileData_Hex_String[MAX_SIZE];
	int fileData_Hex_String_Size;
	unsigned char fhexByteCount[MAX_SIZE];
	unsigned char fhexAddress1[MAX_SIZE];
	unsigned char fhexAddress2[MAX_SIZE];
	unsigned char fhexRecordType[MAX_SIZE];
	unsigned char fhexCheckSum[MAX_SIZE];

};



///////////////////// PROGRAM ////////////////////////////

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


void UUencode(unsigned char fileData_Hex_String[], int hexDataCharCount)
{
	// ASCII->HEX->UUE Test Strings
	// ASCII: The witch Lilith knows my soul.
	// HEX: {0x54, 0x68, 0x65, 0x20, 0x77, 0x69, 0x74, 0x63, 0x68, 0x20, 0x4c, 0x69, 0x6c, 0x69, 0x74 0x68, 0x20, 0x6b, 0x6e, 0x6f, 0x77, 0x73, 0x20, 0x6d, 0x79, 0x20, 0x73, 0x6f, 0x75, 0x6c, 0x2e};
	// UUE-ASCII: ?5&AE('=I=&-H($QI;&ET:"!K;F]W<R!M>2!S;W5L+@
	// UUE-HEX:{35 26 41 45 28 27 3d 49 3d 26 2d 48 28 24 51 49 3b 26 45 54 3a 22 21 4b 3b 46 5d 57 3c 52 21 4d 3e 32 21 53 3b 57 35 4c 2b 40}
	//unsigned char testUUE_HEX[] = {0x54, 0x68, 0x65, 0x20, 0x63, 0x61, 0x72, 0x00};

	unsigned char UUE_Encoded_String[MAX_SIZE];
	unsigned char b[3];
	unsigned char d[4];

	int paddedIndex = 0;
	int UUE_Encoded_String_Index = 0;

	for(int hexDataIndex = 0;  hexDataIndex < hexDataCharCount; hexDataIndex)
	{
		// Load chars or nulls
		for (int i = 0; i < 3; i++)
		{

			if (hexDataIndex < hexDataCharCount)
			{
				b[i] = fileData_Hex_String[hexDataIndex];	
			}
			else
			{
				b[i] = 0;
				paddedIndex++;
			}
			hexDataIndex++;
		}
		
		// UUEncode
		d[0] = (((b[0] >> 2) & 0x3f) + ' ');
		d[1] = ((((b[0] << 4) | ((b[1] >> 4) & 0x0f)) & 0x3f) + ' ');
		d[2] = ((((b[1] << 2) | ((b[2] >> 6) & 0x03)) & 0x3f) + ' ');
		d[3] = ((b[2] & 0x3f) + ' ');
		
		// Put the UUEncoded chars into their own string.
		for (int i = 0; i < 4; i++)
		{
			UUE_Encoded_String[UUE_Encoded_String_Index] = d[i];
			UUE_Encoded_String_Index++;
		}
	}
	
	// ADD FOR-LOOP TO DIVIDE UUE DATA INTO 61 CHAR STRINGS.

	/*
	for (int i = 0; i < (UUE_Encoded_String_Index-paddedIndex); ++i)
	{
		printf(" 0x%2x \n", UUE_Encoded_String[i]);
	}
	*/
}

void fileSizer()
{
	while((fgetc (fileIn)) != EOF)
	{
		fileSize++;
	}
	rewind(fileIn);
}

/***************************** Ascii2Hex ********************************/
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
        return (unsigned char)(c - 'a' + 10);
    }
	//printf("\n !!! Bad Character: 0x%2x in file at totalCharsRead=%d !!!\n\n", c, totalCharsRead);
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
struct hexFile hexFileToCharArray()
{
	//To count through all characters in the file.
	int i = 0;
	int hexDataIndex = 0;
	int charsThisLine;
	//Holds line count.
	int hexFileLineCount = 0;
	
	struct hexFile hexFile;

	//Loop through each character until EOF.
	while(totalCharsRead  < fileSize){
		
		//BYTE COUNT
		hexFile.fhexByteCount[i] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		hexFile.fhexAddress1[i] = readByte();
		
		//ADDRESS2
		hexFile.fhexAddress2[i] = readByte();
		
		//addressCombine();
		
		//RECORD TYPE
		hexFile.fhexRecordType[i] = readByte();	
		
		//Throws the byte count (data bytes in this line) into an integer.
		charsThisLine = hexFile.fhexByteCount[i];

		//////// DATA ///////////////////
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
		
		//////// CHECK SUM //////////////
		if (charToPut != 0xFF){
			hexFile.fhexCheckSum[i] = readByte();
		}
		i++;
		
		
	}
	//Load the size of the data array.
	hexFileLineCount = i;
	return hexFile;
}

int main(int argc, char *argv[])
{
	//For setting state of DTR/CTS.
    unsigned char DTR_Switch;
	unsigned char CTS_Switch;

	struct hexFile hexFile;
	//Used by FTD2XX
    DWORD bytes;

	//If the user fails to give us two arguments yell at him.	
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s <readfile1>\n", argv[0] );
		exit ( EXIT_FAILURE );
	}

	
 
	//Open file using command-line info; for reading.
	fileIn = open_file ( argv[1], "rb" );

	// Initialize, open device, set bitbang mode w/5 outputs
    //if(FT_Open(0, &handle) != FT_OK) {
    //    puts("Can't open device");
    //    return 1;
    //}

	//Set up pins we will use.
	//FT_SetBitMode(handle, PIN_DTR | PIN_CTS, 1);
	//Setup serial port, even though we are banging.
    //FT_SetBaudRate(handle, 9600);  //* Actually 9600 * 16

	//Sizes file to be used in data handling.
	fileSizer();
	
	//Convert file to one long char array.
	hexFile = hexFileToCharArray();

	//Used for indexing data bytes in for-loop.
	int dataPerLineIndex = 0;

	UUencode(hexFile.fileData_Hex_String, hexFile.fileData_Hex_String_Size);

	for (int totalUUEData_Index = 0; totalUUEData_Index < hexFile.fileData_Hex_String_Size; totalUUEData_Index)
	{
		for (int i = 0; i < 60; i++)
		{
			printf(" 0x%2x", hexFile.fileData_Hex_String[totalUUEData_Index]);
			totalUUEData_Index++;
		}
		printf("\nEND PAGE\n\n");
	}

	//Prints out each byte, one at a time.
	for (int i = 0; i < hexFile.fileData_Hex_String_Size; i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		//FT_Write(handle, &fileData_Hex_String[i], (DWORD)sizeof(fileData_Hex_String[i]), &bytes);
		//printf(" #%i", (i));
		Sleep(1);
	}	

	//Let's close the serial port.
	//if(FT_Close(handle) != FT_OK) {
    //    puts("Can't close device");
    //    return 1;
    //}

	//Close files.
	fclose ( fileIn );

} // END PROGRAM