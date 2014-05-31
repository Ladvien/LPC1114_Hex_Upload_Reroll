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
unsigned char UUEData[32000];

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

/* ENC is the basic 1 character encoding function to make a char printing */
#define	ENC(c) ((c) ? ((c) & 077) + ' ': '`')
void
encode()
{
	unsigned char* UUE_Chop[1];
	unsigned char UUE_Seam[1];
	unsigned char UUEBuff[32000];
	
	int UUEIndex = 0;
	
	while (UUEIndex < dataSize){
		//if > 4 bytes -- handles last few bytes.
		
		//                        1    1
		//Load the first byte [000000]00
		UUEBuff[UUEIndex] = arrayForFileChar[UUEIndex];
		//Make it senary.
		UUE_Chop[0] = &UUEBuff[UUEIndex];  //Load byte one for chopping.
		UUEBuff[UUEIndex] >> 2;  //Clip right two-bits.
		UUEBuff[UUEIndex] + 32;  //Add 32 for encoding.
		//printf(" %2x", UUEBuff[UUEIndex]); //Print first senary.
		UUEIndex++;  //Move forward one byte.
		
		//                                        1    2    2
		//Two of first, first four of the second [00 0000]0000
		UUEBuff[UUEIndex] = arrayForFileChar[UUEIndex]; //Load second byte.
		//Make it senary.
		UUE_Chop[1] = &UUEBuff[UUEIndex];  //Load byte two for chopping.
		UUEBuff[UUEIndex] >> 4;  //Get four bits from second chop in place.
		UUE_Seam[0] = *UUE_Chop[0]; //Load chop one into seam.
		UUE_Seam[0] << 6;  //Get rid of left bits from byte one.
		UUE_Seam[0] >> 2; //Move them back into a place to be OR'ed with second chop
		UUEBuff[UUEIndex] |= UUE_Seam[0]; //[00 0000]0000
		UUEBuff[UUEIndex] + 32;  //Add 32 for encoding.
		//printf(" %2x", UUEBuff[UUEIndex]); //Print first senary.
		
		UUEIndex++;  //Move forward one byte.
		
		//                                        2    3    3
		//Two of first, first four of the second [0000 00]000000 
		UUEBuff[UUEIndex] = arrayForFileChar[UUEIndex]; //Load third byte.
		//Make it senary.
		UUE_Chop[0] = &UUEBuff[UUEIndex];  //Load byte three for chopping.
		UUEBuff[UUEIndex] >> 6; //Get two bits from third byte.
		UUE_Seam[1] = *UUE_Chop[1]; //Load chop two into seam.
		UUE_Seam[1] << 2; //Get chop two into place.
		UUEBuff[UUEIndex] |= UUE_Seam[0]; //[00 0000]0000
		UUEBuff[UUEIndex] + 32;  //Add 32 for encoding.
		//printf(" %2x", UUEBuff[UUEIndex]); //Print first senary.
		
		UUEIndex++;  //Move forward one byte.
		
		//                                         3    3
		//Two of first, first four of the second  00[000000] 
		UUEBuff[UUEIndex] = arrayForFileChar[UUEIndex]; //Load third byte.
		UUEBuff[UUEIndex] << 2; //Get rid of two left bits
		UUEBuff[UUEIndex] >> 2; //Put it back in place.
		UUEBuff[UUEIndex] + 32;  //Add 32 for encoding.
		//printf(" %2x", UUEBuff[UUEIndex]);
		
		UUEIndex++;  //Move forward one byte.
		
	}
	//printf("\nUUE Char: %2x\n", ch);
	
		/*
	while ((n = fread(buf, 1, 45, stdin))) {
		ch = ENC(n);
		
		if (fputc(ch, output) == EOF)
			break;
		for (p = buf; n > 0; n -= 3, p += 3) {
			// Pad with nulls if not a multiple of 3. 
			if (n < 3) {
				p[2] = '\0';
				if (n < 2)
					p[1] = '\0';
			}
			ch = *p >> 2;
			ch = ENC(ch);
			if (fputc(ch, output) == EOF)
				break;
			ch = ((*p << 4) & 060) | ((p[1] >> 4) & 017);
			ch = ENC(ch);
			if (fputc(ch, output) == EOF)
				break;
			ch = ((p[1] << 2) & 074) | ((p[2] >> 6) & 03);
			ch = ENC(ch);
			if (fputc(ch, output) == EOF)
				break;
			ch = p[2] & 077;
			ch = ENC(ch);
			if (fputc(ch, output) == EOF)
				break;
		}
		if (fputc('\n', output) == EOF)
			break;
	}
	if (ferror(stdin))
		errx(1, "read error");
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
		
		//Throws the byte count (data bytes in this line) into an integer.
		charsThisLine = fhexByteCount[i];


		
		//////// DATA ///////////////////
		while (hexDataIndex != charsThisLine && totalCharsRead  < fileSize && charsThisLine != 0x00)
		{
			//Store the completed hex value in the char array.
			arrayForFileChar[dataSize] = readByte();
			//Index for data.
			dataSize++;
			//Index for loop.
			hexDataIndex++;
		}
		//Reset loop index.
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

	//Sizes file to be used in data handling.
	fileSizer();

	//Convert file to one long char array.
	hexFileToCharArray();
	
	printf("\n\nHex output file\n\n");
	
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
			//printf("%2x ", arrayForFileChar[dataPerLineIndex]);
			//Increases overall data index.
			dataPerLineIndex++;
		}
		//Print data bytes on this line.
		//printf(" #%i\n", (charsThisLine));

	}
	encode();
	//Prints fileSize and totalCharsRead for user.
	//printf("\nFile Size : %i\n", fileSize);
	//printf("Chars Read: %i\n", ((totalCharsRead)));
			
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