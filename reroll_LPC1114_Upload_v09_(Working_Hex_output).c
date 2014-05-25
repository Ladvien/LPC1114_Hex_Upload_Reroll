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

//Reading characters from a file.
unsigned char charToPut;

//File to be loaded.	
FILE *fileIn;

/////////// Hex file to UUE stuff /////////////////
//To hold file hex values.
unsigned char arrayForFileChar[32000];
#define HEX_DATA_INDEX_LIMIT 16
unsigned char fhexByteCount[32000];
unsigned char fhexAddress1[32000];
unsigned char fhexAddress2[32000];
unsigned char fhexRecordType[32000];
unsigned char fhexCheckSum[32000];
int hexFileLineCount = 0;

/////////// Hex file to UUE stuff /////////////////
//Used to know the size of arrayForFileChar.
int fileSize;


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
    return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
} 

void clearSpecChar()
{
		//Removes "\r"
		if (charToPut == '\r'){
			charToPut = fgetc (fileIn);
		}
		//Removes "\n"
		if (charToPut == '\n'){
			charToPut = fgetc (fileIn);
		}
		//Removes ":"
		if (charToPut == ':'){
			charToPut = fgetc (fileIn);
		}
}



//Convert file to one long char array.
int hexFileToCharArray()
{
	unsigned char hexValue;

	//To count through all characters in the file.
	int i = 0;
	int hexDataIndex = 0;
	
	//Loop through each character until EOF.
	while((charToPut = fgetc (fileIn))  != 0xFF ){
		
		//Clears ':', '\r', '\n'
		clearSpecChar();
				
		//////// BYTE COUNT //////////////
		//Put first nibble in.
		hexValue = (Ascii2Hex(charToPut));
		//Slide the nibble.
		hexValue <<= 4;
		//Get second nibble.
		charToPut = fgetc (fileIn);
		clearSpecChar();
		
		//Put the nibbles together.
		hexValue |= (Ascii2Hex(charToPut));
		
		//Start code
		fhexByteCount[i] = hexValue;
		
		//////// ADDRESS1 //////////////
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
		
		//ADDRESS1
		fhexAddress1[i] = hexValue;
		
		//////// ADDRESS2 //////////////
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
		
		//ADDRESS2
		fhexAddress2[i] = hexValue;
		
		//////// RECORD TYPE //////////////
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
		
		//RECORD TYPE
		fhexRecordType[i] = hexValue;	
		
		//////// DATA ///////////////////
		
		while (hexDataIndex < 15 && charToPut != 0xFF)
		{
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
			hexValue |= (Ascii2Hex(charToPut));
			
			//Store the completed hex value in the char array.
			arrayForFileChar[fileSize] = (hexValue);
			fileSize++;
			hexDataIndex++;
		}
		hexDataIndex=0;
		
		//////// CHECK SUM //////////////
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
		
		//RECORD TYPE
		fhexCheckSum[i] = hexValue;
		
		putchar(1);
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
	fileIn = open_file ( argv[1], "r" );


	// Initialize, open device, set bitbang mode w/5 outputs
    if(FT_Open(0, &handle) != FT_OK) {
        puts("Can't open device");
        return 1;
    }

	//Set up pins we will use.
	//FT_SetBitMode(handle, PIN_DTR | PIN_CTS, 1);
	//Setup serial port, even though we are banging.
    FT_SetBaudRate(handle, 9600);  //* Actually 9600 * 16

	//Convert file to one long char array.
	hexFileToCharArray();

	//Prints out each byte, one at a time.
	for (int i = 0; i < fileSize; i++){
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