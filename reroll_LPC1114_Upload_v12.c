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

//Serial Port handle.
FT_HANDLE handle;


//File to be loaded.	
FILE *fileIn;
int fileSize = 0;

/////////// Hex file to UUE stuff /////////////////
//To hold file hex values.
unsigned char arrayForFileChar[32000];

unsigned char fhexByteCount[32000];
unsigned char fhexAddress1[32000];
unsigned char fhexAddress2[32000];
unsigned char fhexRecordType[32000];
unsigned char fhexCheckSum[32000];
//Holds combined nibbles.
unsigned char hexValue;

//Reading characters from a file.
unsigned char charToPut;
//Holds line count.
int hexFileLineCount = 0;
//Total bytesRead.
int totalCharsRead = 0;
char * fileCharPoint;
int charsThisLine;


/////////////////////////////////////////////////

//Used to know the size of arrayForFileChar.
int dataSize;


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
	printf("\n !!! Bad Character: 0x%2x in file at totalCharsRead=%d !!!\n\n", c, totalCharsRead);
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
int hexFileToCharArray()
{
	//To count through all characters in the file.
	int i = 0;
	int hexDataIndex = 0;
	
	//Loop through each character until EOF.
	while(totalCharsRead  < fileSize){
		
		//BYTE COUNT
		fhexByteCount[i] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		fhexAddress1[i] = readByte();
		
		//ADDRESS2
		fhexAddress2[i] = readByte();
		
		//addressCombine();
		
		//RECORD TYPE
		fhexRecordType[i] = readByte();	
		
		
		charsThisLine = fhexByteCount[i];
		if(charsThisLine > 16){
			fhexByteCount[i] <<=4;
			fhexByteCount[i] >>=4;
			charsThisLine = fhexByteCount[i];
			printf("\n\nBOO!\n\n");
		}
		
		//////// DATA ///////////////////
		while (hexDataIndex != charsThisLine && totalCharsRead  < fileSize && charsThisLine != 0x00)
		{
			//Store the completed hex value in the char array.
			arrayForFileChar[dataSize] = readByte();
			dataSize++;
			hexDataIndex++;
		}
		hexDataIndex=0;
		
		//////// CHECK SUM //////////////
		if (charToPut != 0xFF){
			fhexCheckSum[i] = readByte();
		}
		i++;
		
		
	}
	//Load the size of the data array.
	hexFileLineCount = (i);
}

int main(int argc, char *argv[])
{
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
	fileIn = open_file ( argv[1], "rb" );

	// Initialize, open device, set bitbang mode w/5 outputs
    if(FT_Open(0, &handle) != FT_OK) {
        puts("Can't open device");
        return 1;
    }

	//Set up pins we will use.
	//FT_SetBitMode(handle, PIN_DTR | PIN_CTS, 1);
	//Setup serial port, even though we are banging.
    FT_SetBaudRate(handle, 9600);  //* Actually 9600 * 16

	fileSizer();

	//Convert file to one long char array.
	hexFileToCharArray();
	
	printf("\n\nHex output file\n\n");
	//printf("\nChars a line: %i\n ", charsThisLine);
	
	//Used for indexing data bytes in for-loop.
	int dataPerLineIndex = 0;
	
	//Loops through each line, until the line count and index are equal.
	for(int lineIndex = 0; lineIndex != hexFileLineCount; ++lineIndex){
		//Converts this line byte count into an integer.
		charsThisLine = fhexByteCount[lineIndex];
		//Loops through printing data bytes until byte count 
		//for this line and index are equal.
		for(int charsOnThisLineIndex = 0; charsOnThisLineIndex != charsThisLine; charsOnThisLineIndex++)
		{
			// Print the bytes.
		   printf("%2x ", arrayForFileChar[dataPerLineIndex]);
		   //Increases overall data index.
		   dataPerLineIndex++;
		}
		//Print data bytes on this line.
		printf(" #%i\n", (charsThisLine));
		//printf("0x%2x \n", fhexByteCount[lineIndex]);
	}
	printf(" %i\n", fileSize);
	printf(" #%i\n", ((totalCharsRead)));
			
	//Prints out each byte, one at a time.
	for (int i = 0; i < dataSize; i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FT_Write(handle, &arrayForFileChar[i], (DWORD)sizeof(arrayForFileChar[i]), &bytes);
		Sleep(1);
	}	

	//Let's close the serial port.
	if(FT_Close(handle) != FT_OK) {
        puts("Can't close device");
        return 1;
    }

	//Close files.
	fclose ( fileIn );

}