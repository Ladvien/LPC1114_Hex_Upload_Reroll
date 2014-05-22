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
char arrayForFileChar[32000];

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
    //ErrorPrintf("Wrong Hex-Nibble %c (%02X)\n", c, c);
    //exit(1);
    return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
} 
		
//Convert file to one long char array.
int hexFileToCharArray()
{
	//To count through all characters in the file.
	int i = 0;
	//Loop through each character until EOF.
	while((charToPut = fgetc (fileIn))  != 0xFF ){
		unsigned char hexValue;
		//Convert the character into hex.
		//hexValue = Ascii2Hex(charToPut);
		puts(""+charToPut);
		
		//Load hex value into char array.
		arrayForFileChar[i] = hexValue;
		i++;
	}
	//Load the size of the data array.
	fileSize=(i-1);
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
	
	for (int i = 0; i < fileSize; i++){
		FT_Write(handle, &arrayForFileChar[i], (DWORD)sizeof(charToPut), &bytes);
	}	
	



	
	/*


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
	
