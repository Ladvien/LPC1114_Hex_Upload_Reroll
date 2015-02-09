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

//To hold file hex values.
unsigned char arrayForFileChar[32000];

//Used to know the size of arrayForFileChar.
int fileSize;


//What sort of Native American we got?
const int E = 1;
#define is_bigendian() ( (*(char*)&E) == 0 )
unsigned int swapped;

unsigned char reverseShort (unsigned char s) {
    unsigned char c1, c2;
    
    if (is_bigendian()) {
        return s;
    } else {
        c1 = s & 255;
        c2 = (s >> 8) & 255;
    
        return (c1 << 8) + c2;
    }
}


void ascToBinary(int character, int *ones)
{
	if(character == 1)
	{
	   printf("1");
	   *ones+=1;
	   return;
	}
	else
	{
		char out;
		if((character%2) == 0)
		{
			 out = '0';
			 character = character/2;
		}
		else
		{
			 out = '1';
			 character = character/2;
			 *ones+=1;

		}
		ascToBinary(character, ones);
		putchar (out);
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
		
//Convert file to one long char array.
int hexFileToCharArray()
{
	unsigned char hexValue1;
	unsigned char hexValue2;
	unsigned char hexValue3;


	//To count through all characters in the file.
	int i = 0;
	//Loop through each character until EOF.
	while((charToPut = fgetc (fileIn))  != 0xFF ){
	
		
		//Removes ":"
		if (charToPut = 0x3A){
			charToPut = fgetc (fileIn);
		}
		
		hexValue1 = Ascii2Hex(charToPut);
		hexValue1 <<= 4;
		charToPut = fgetc (fileIn);
		hexValue1 |= Ascii2Hex(charToPut);
		
		//hexValue3 = (hexValue2>>4) | (hexValue1<<4);
		
		//Convert the character into hex.
		//hexValue = Ascii2Hex(charToPut);
		//charToPut = fgetc (fileIn);
		//halfHex = Ascii2Hex(charToPut);
		//halfHex |= hexValue << 4;
		//hexValue |= halfHex;
		//Load hex value into char array.
		arrayForFileChar[i] = reverseShort(hexValue1);
		i++;

		
	}
	//Load the size of the data array.
	fileSize=(i);
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
	
	if (is_bigendian()) {
        puts("Is Big Endian");
    } else {
        printf("Is Little Endian");
    }
	
	//Prints out each byte, one at a time.
	for (int i = 0; i < fileSize; i++){
		FT_Write(handle, &arrayForFileChar[i], (DWORD)sizeof(arrayForFileChar[i]), &bytes);
		Sleep(5);
	}	
	

	
	/*
	
	hex_val = (ascii2hex(ch1)<<4) | ascii2hex(ch2)

    //Write the DTR pin LOW.  This prepares to set the chip into bootloader mode.
    FT_Write(handle, &DTR_Switch, (DWORD)sizeof(DTR_Switch), &bytes);
	//Wait 50ms before reseting the chip.
	Sleep(20);
	//Write CTS pin LOW.  This resets the chip and puts it into bootloader.
	FT_Write(handle, &CTS_Switch, (DWORD)sizeof(CTS_Switch), &bytes);
	//Let's wait a second to make sure we are stable.
	Sleep(100);
	//XOR both pin states.
	DTR_Switch ^= PIN_DTR;
	CTS_Switch ^= PIN_CTS;
	//Write the DTR pin HIGH. Preparing for resetting into run mode.
	FT_Write(handle, &DTR_Switch, (DWORD)sizeof(DTR_Switch), &bytes);
	//Let's make sure the ISP pin is grounded.
	Sleep(20); 
	FT_Write(handle, &CTS_Switch, (DWORD)sizeof(CTS_Switch), &bytes);
	Sleep(20);
	// End of FTDI DTR/CTS flip
	*/
	
	//Let's close the serial port.
	if(FT_Close(handle) != FT_OK) {
        puts("Can't close device");
        return 1;
    }
	
	//Close files.
	fclose ( fileIn );
		
}//The end.

/*
bdk6: never been there :-)
bdk6: been there lots of times
Delete Ladvien: Like the Congo after a rain.
Delete Ladvien: Absolutely.
Delete Ladvien: Ah. Yes, I noticed that's what lpc21isp did. Was shooting for that, but got stuck on printing values correctly.
bdk6: clear as mud?
bdk6: a one line comment before each section // this reads leading colon // this reads address ...// this gets the checksum
bdk6: have code in while loop read a colon, then address, etc. Break it into separate steps that follow the structure
Ladvien: Ah, yessir.
Ladvien: Er? As in characters per line, EOF, etc?
bdk6: a line of hex file is colon, address, type code, length, data, checksum
bdk6: have code in while loop read a colon, then address, etc. Break it into separate steps that follow the structure
Delete Ladvien: Ah, yessir.
Delete Ladvien: Er? As in characters per line, EOF, etc?
bdk6: a line of hex file is colon, address, type code, length, data, checksum
bdk6: have the code structure follow the file structure
Delete Ladvien: Yes, sir?
Delete Ladvien: I've still got to streal... erm, write a UUE conversion function too
bdk6: a word of advice when parsing files like that
Ladvien: Well, bits.
bdk6: no problem. You've almost got it. Looks pretty good.
Ladvien: A bit. But a bit is what got me in trouble in the first place.
Ladvien: I guess that means back to work for me. Bdk6, as always, thank you for the guidance.
bdk6: If it makes you feel any better I've had co-workers do similar things
Ladvien: *looks sheepish* Yes, sir.
bdk6: gotta get all the right bits there first ;-)
Ladvien: I'd assumed I could do bitwise on hex values.
bdk6: yes, kariloy, a BLAST!
bdk6: hex_val = (ascii2hex(ch1)<<4) | ascii2hex(ch2)
kariloy: I'm going to have a blast with them \o/
Ladvien: Ah. Yes, I think the only bitwise I've done has been in AVR, specifically, their Studio.
bdk6: correct
Ladvien: Ah, so, convert to binary then OR to the left?
bdk6: also, I don't think you are skipping over and
bdk6: on their way to your house, kariloy :-)
kariloy: what about bomb-bots?
bdk6: those are ascii chars 0,1,...A,B..F Gotta convert to binary first 0000 to 1111
Ladvien: Damnit.
bdk6: you read characters from the file, shift one left and or it with second
Ladvien: Yes, sir?
bdk6: Lad, in your while loop on line 84 +
Ladvien: Make bots, not bombs.
bdk6: later
Ladvien: o/ Have fun, miss!
Roxanna77: back in a bit...
Roxanna77: going to head down to the lab and do some tests on breadboard...
Ladvien: The Swift kind or CPU kind?
Roxanna77: @lad, no problem, was looking up Endians :) now I know!
Ladvien: Two FTDI cables, one monitoring the other's feed.
Ladvien: Yes, sir, even.
bdk6: you are on windows
	*/
