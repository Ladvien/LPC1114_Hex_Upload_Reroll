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
FILE *hexDataFile;
FILE *UUEDataFile;

int fileSize = 0;


//Reading characters from a file.
unsigned char charToPut;

//Total bytesRead.
int totalCharsRead = 0;

struct hexFile {
	//To hold file hex values.
	unsigned char fileData_Hex_String[MAX_SIZE];
	int fileData_Hex_String_Size;
	unsigned char fhexByteCount[MAX_SIZE];
	int hexFileLineCount;
	unsigned char fhexAddress1[MAX_SIZE];
	unsigned char fhexAddress2[MAX_SIZE];
	unsigned char fhexRecordType[MAX_SIZE];
	unsigned char fhexCheckSum[MAX_SIZE];

};

struct UUE_Data{
	unsigned char UUE_Encoded_String[MAX_SIZE];
	unsigned char b[3];
	unsigned char d[4];

	int paddedIndex;
	int UUE_Encoded_String_Index;
};

///////////////////// PROGRAM ////////////////////////////
////////////////////// START /////////////////////////////

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
struct hexFile hexFileToCharArray()
{
	//To count through all characters in the file.
	int i = 0;
	int hexDataIndex = 0;
	int charsThisLine;
	//Holds line count.
	
	
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

		hexFile.hexFileLineCount++;
		i++;
		
		
	}
	
	
	return hexFile;
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
					fprintf(hexDataFile, "%2x", fileData_Hex_String[totalDataIndex]);
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

void writeUUEDataTofile(unsigned char fileData_Hex_String[], int hexDataCharCount)
{
	UUEDataFile = open_file ("uueFile.uue", "w" );
	if (UUEDataFile == NULL) {
    	printf("I couldn't open uueFile.uue for writing.\n");
    	exit(0);
    }
}



int main(int argc, char *argv[])
{
	// Stores hexFile data.
	struct hexFile hexFile;

	// Stores UUE data.
	struct UUE_Data UUE_Data;

	//For setting state of DTR/CTS.
    unsigned char DTR_Switch;
	unsigned char CTS_Switch;

	//Used by FTD2XX
    DWORD bytes;

	//If the user fails to give us two arguments yell at him.	
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s <readfile1>\n", argv[0] );
		exit ( EXIT_FAILURE );
	}

	//Open file using command-line info; for reading.
	fileIn = open_file (argv[1], "rb" );
	
	// Initialize, open device, set bitbang mode w/5 outputs
    if(FT_Open(0, &handle) != FT_OK) {
        puts("Can't open device");
        return 1;
    }

	//Set up pins we will use.
	FT_SetBitMode(handle, PIN_DTR | PIN_CTS, 1);
	//Setup serial port, even though we are banging.
    FT_SetBaudRate(handle, 9600);  //* Actually 9600 * 16

	//Sizes file to be used in data handling.
	fileSizer();
	
	//Convert file to one long char array.
	hexFile = hexFileToCharArray();

	//int dataLinesInFile = sizeof(hexFile.hexFileLineCount);
	printf("%i\n", hexFile.hexFileLineCount);

	// Write hex string back to a file.  Used for debugging.
	writeHexDataTofile(hexFile.fileData_Hex_String, hexFile.fileData_Hex_String_Size, hexFile.fhexByteCount, hexFile.hexFileLineCount);
	
	// Convert hex data to UUE.
	UUE_Data = UUencode();

	//Used for indexing data bytes in for-loop.
	int dataPerLineIndex = 0;
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
	for (int i = 0; i < UUE_Data.UUE_Encoded_String_Index; i++){
		unsigned char * bufPtr = UUE_Data.UUE_Encoded_String[i];
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FT_Write(handle, UUE_Data.UUE_Encoded_String[i], (DWORD)sizeof(UUE_Data.UUE_Encoded_String[i]), &bytes);
		printf(" %2x", UUE_Data.UUE_Encoded_String[i]);
		Sleep(5);
	}	

	//Let's close the serial port.
	if(FT_Close(handle) != FT_OK) {
        puts("Can't close device");
        return 1;
    }

	//Close files.
	fclose ( fileIn );
	fclose ( UUEDataFile );
	fclose ( hexDataFile );
} // END PROGRAM