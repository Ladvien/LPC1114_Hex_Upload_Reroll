//LPC1114 Reset program.  Meant to come before using lpc21isp.  Should allow for 
//popular FTDI breakouts, like Sparkfun's, to be used as a serial programmer for 
//the LPC1114.  Shooting for no manual reset.
#include "main.h"
#include "FTDI_helper.h"

//Serial Port handle.
//Used by FTD2XX
//FT_HANDLE handle = NULL;
FT_STATUS FT_status;

DWORD EventDWord;
DWORD TxBytes;
DWORD BytesWritten;
DWORD RxBytes;
DWORD BytesReceived;

FT_DEVICE_LIST_INFO_NODE *devInfo;
bool FTDI_open_flag = false;


uint8_t RawRxBuffer[2048];
uint8_t ParsedRxBuffer[2048];

DWORD * ptr_bytes_written = &BytesWritten;

char file_name[40];

//File to be loaded.	
FILE *hexDataFile;
FILE *UUEDataFile;
FILE *debug;

//Reading characters from a file.
uint8_t charToPut;

//Total bytesRead.
int totalCharsRead = 0;

int command_response_code = 0;

///////////////////// PROGRAM ////////////////////////////
////////////////////// START /////////////////////////////

int main(int argc, char *argv[])
{
	// Setup console.
	clearConsole();


	strncpy(file_name, argv[1], 39);
	file_name[39] = '\n';

	main_menu();
	//If the user fails to give us two arguments yell at him.	
	if ( argc != 2 ) {
		fprintf ( stderr, "Usage: %s <readfile1>\n", argv[0] );
		exit ( EXIT_FAILURE );
	}
} // END PROGRAM

void main_menu()
{
	char char_choice[3];
	int int_choice = 0;

	do
	{
		system("cls");
		startScreen();
		printf("\n");	
		printf("Vorpal Hoff -- Main Menu: \n\n");
		printf("1. Quick Connect\n");
		printf("2. FTDI Menu\n");
		printf("3. Open HM-1X Menu\n");
		printf("4. Connect LPC\n");
		printf("5. Program Chip\n");
		printf("6. Erase LPC\n");
		printf("7. Decode UUE debug file\n");
		printf("8. Parse hex-file and write to debug.hex\n");
		//printf("8. \n");
		printf("9. Exit\n");

		scanf("%s", char_choice);
		int_choice = atoi(char_choice);

		switch (int_choice)
		{
			case 1:
				quick_connect();
				break;
			case 2:
				ftdi_menu();
				break;
			case 3:
				HM_1X_main_menu();
				break; 
			case 4:

				break;
			case 5:
				program_chip(file_name);
			    break;
			case 6:
			    break;
			case 8:
				debug_hex_file(file_name);
			    break;
			case 9:
				shut_down();    
			    break;
			default:printf("Wrong choice. Enter Again");
			    break;
		}
	}while(int_choice !=99);
	
}

void debug_hex_file(char file_name[])
{
	FILE *hex_file;

	// Stores file size.
	int fileSize = 0;
	int combined_address[2048];
	struct Data data;
	struct write write;
	uint8_t UUE_array[32768];

	debug = open_file ("debug.hex", "w" );
	
	// Local for FTDI State Machine.
	//FTDI_state FTDI_Operation = RX_CLOSE;

	//Open file using command-line info; for reading.
	hex_file = open_file (file_name, "rb" );

	// Sizes file to be used in data handling.
	fileSize = file_sizer(hex_file);

	// Load the data from file
	data.HEX_array_size = hex_file_to_array(hex_file, data.HEX_array);
	
	int UUE_array_size = 0;

	UUE_array_size = UUEncode(UUE_array, data.HEX_array, data.HEX_array_size);

 	printf("\n\n");


	int UUE_index = 0;

	while(UUE_index < UUE_array_size)
	{
		printf("%c", UUE_array[UUE_index]);
		UUE_index++;
		
	}

	int hex_index = 0;
	int totalDataIndex = 0;

	hexDataFile = open_file ("hexFile.hex", "w" );
		if (hexDataFile == NULL) {
		printf("I couldn't open hexFile.hex for writing.\n");
		exit(0);
	}

	int line_index = 0;
	while(hex_index < data.HEX_array_size)
	{
		for (int i = 0; i < 16; ++i && hex_index < data.HEX_array_size)
		{
			fprintf(hexDataFile, "%02X", data.HEX_array[hex_index]);
			//printf("%02X", data.HEX_array[hex_index]);
			hex_index++;
		}
		fprintf(hexDataFile, "  %i\n", combined_address[line_index]);
		//printf("     %i\n", combined_address[line_index]);
		line_index++;		
	}



	//printf("\n\n" );
	shut_down();
}

void shut_down()
{
	fclose ( UUEDataFile );
	fclose ( hexDataFile );
	fclose ( debug );
	clearConsole();
	printf("\n\"Hot glue!!\" --Hoff70\n  1968 - 2015\n");
	//printf("In memory of Johnathan Hoffman\n1968 - 2015\n");

	exit(0);
}

void program_chip(char file_name[])
{
	timer();

	//For setting state of DTR/CTS.
	//uint8_t DTR_Switch;
	//uint8_t CTS_Switch;
	FILE *hex_file;
	// Holds the raw hex data, straight from the file.
	uint8_t UUE_data_array[MAX_SIZE];

	// Array for decoded chunk.
	uint8_t decoded_bytes[3];

	// Stores file size.
	int fileSize = 0;

	struct Data data;
	struct write write;
	int combined_address[2048];
	debug = open_file ("debug.hex", "w" );
	
	// ISP uses RAM from 0x1000 017C to 0x1000 17F
	write.ram_address = 0x1000017C;

	// Local for FTDI State Machine.
	//FTDI_state FTDI_Operation = RX_CLOSE;

	//Open file using command-line info; for reading.
	hex_file = open_file (file_name, "rb" );

	// Sizes file to be used in data handling.
	fileSize = file_sizer(hex_file);

	// Load the data from file
	data.HEX_array_size = hex_file_to_array(hex_file, data.HEX_array);
	data.HEX_array_size = make_array_multiple_of_four(data.HEX_array, data.HEX_array_size);
	write.sectors_needed = sectors_needed(data.HEX_array_size);

	// Start with last sector needed.
	write.sector_index = (write.sectors_needed - 1);
	write.sector_to_write = ((write.sector_index) * 4096);
	write.Flash_address = write.sector_to_write;

	printf("Sectors needed %i\n", write.sectors_needed);

	// Write hex string back to a file.  Used for debugging.
	writeHexDataTofile(data);

	// %`^H
	decode_three(decoded_bytes, '%','`','^','H');

	// Let's wake the device chain (FTDI, HM-10, HM-10, LPC)
	bool HM_1X_awake = ping();

	printf("Mid: %.3f\n", timer());

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
	tx_chars("U 23130\n", sizeof("U 23130\n"), PRINT, 0);
	Sleep(120);
	rx(PARSE, PRINT);

	// Clear the entire chip.
	erase_chip(write);

	validity_checksum(write, data);

	// Sectors
	// 16 pages (128) * # of sectors (4096)
	for (int i = 0; i < (16 * write.sectors_needed); ++i)
	{
		// UUEncode 2 pages (512 bytes).  Returns UUE character count (~1033)
		write = prepare_page_to_write(write, data);
		write = write_page_to_ram(write, data);
		write = ram_to_flash(write, data);
	}

	printf("\n\nUpload time in seconds: %.3f\n", timer());

	set_RUN_mode(NO_PRINT);
	/*
	// Read memory
	txString("R 268436224 4\n", sizeof("R 268436224 4\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, PRINT);
	txString("OK\n", sizeof("OK\n"), PRINT, 0);
	Sleep(500);
	rx(PARSE, PRINT);

	*/

	//Close files.
	fclose ( hex_file );
	fclose ( UUEDataFile );
	fclose ( hexDataFile );
	fclose ( debug );
	clearConsole();
}

///////////// FTDI  //////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

uint8_t rx(bool parse, bool printOrNot)
{	
	// 0 = unsuccessful, 1 = LPC, 2 = HM-10
	int device_and_success = 0;
	//FT_SetTimeouts(handle, timeout, 0);
	FT_GetStatus(devInfo[connected_device_num].ftHandle, &RxBytes, &TxBytes, &EventDWord);

	setTextGreen();

	if (RxBytes > 0) {

		FT_status = FT_Read(devInfo[connected_device_num].ftHandle,RawRxBuffer,RxBytes,&BytesReceived);
		if (FT_status == FT_OK) {

			if(parse)
			{
				device_and_success = parserx();
			}
			else
			{
				memcpy(ParsedRxBuffer, RawRxBuffer, sizeof(RawRxBuffer)+1);
				device_and_success = 1;
			}

			clearConsole();
			
			if(printOrNot)
			{
				fprintf(debug, "%s", ParsedRxBuffer);	
				setTextGreen();
				printf("%s\n", ParsedRxBuffer);
				clearConsole();
				clearBuffers();
			}
	
			if(device_and_success > 0){return device_and_success;}

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
		uint8_t *OK_RES_LPC = strstr(RawRxBuffer, "OK");

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
			memcpy(ParsedRxBuffer, RawRxBuffer, sizeof(RawRxBuffer)+1);
			
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
			//command_response(command_response);
			successful = 1;
		}

		// Is it an HM-10?
		else if (RawRxBuffer[0] == 'O' && RawRxBuffer[1] == 'K' && RawRxBuffer[2] == '+')
		{
			memcpy(ParsedRxBuffer, RawRxBuffer, sizeof(RawRxBuffer)+1);
			// HM-10 responded.
			successful=2;
		}
		// Is it a command response from LPC?
		if (OK_RES_LPC != '\0')
		{
			memcpy(ParsedRxBuffer, RawRxBuffer, sizeof(RawRxBuffer)+1);
			// HM-10 responded.
			successful=3;
		}

		// If the RawRxBuffer data does not contain "\r\n" or "OK+" strings.
		else
		{
			memcpy(ParsedRxBuffer, RawRxBuffer, sizeof(RawRxBuffer)+1);
			successful=0;
		}

		// Clear RawRxBuffer
		for (int i = 0; i != sizeof(RawRxBuffer); ++i)
		{
			RawRxBuffer[i] = 0x00;
		}
		return successful;
}


uint8_t tx_chars(char string[], int txString_size, bool printOrNot, int frequency_of_tx_char)
{
	uint8_t FTWrite_Check;

	for (int i = 0; i < (txString_size-1); i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FTWrite_Check = FT_Write(devInfo[connected_device_num].ftHandle, &string[i], (DWORD)sizeof(string[i]), ptr_bytes_written);
		if (FTWrite_Check != FT_OK)
		{
			setTextRed();
			printf("Bad write!\n");
			clearConsole();
		}
		Sleep(frequency_of_tx_char);
		fprintf(debug, "%c", string[i]);	
		while((txString_size) < *ptr_bytes_written){Sleep(1000);}
		
		if(printOrNot)
		{
			setTextRed();
			printf("%C", string[i]);
		}
	}	
	clearConsole();
	return 0;
}

uint8_t tx_data(uint8_t string[], int txString_size, bool printOrNot, int frequency_of_tx_char)
{
	uint8_t FTWrite_Check;

	for (int i = 0; i < (txString_size-1); i++){
		//This should print just data (ie, no Start Code, Byte Count, Address, Record type, or Checksum).
		FTWrite_Check = FT_Write(devInfo[connected_device_num].ftHandle, &string[i], (DWORD)sizeof(string[i]), ptr_bytes_written);
		if (FTWrite_Check != FT_OK)
		{
			setTextRed();
			printf("Bad write!\n");
			clearConsole();
		}
		Sleep(frequency_of_tx_char);
		fprintf(debug, "%c", string[i]);	
		while((txString_size) < *ptr_bytes_written){Sleep(1000);}
		
		if(printOrNot)
		{
			setTextRed();
			printf("%C", string[i]);
		}
	}	
	clearConsole();
	return 0;
}

// Output files, for debugging purposes.
///////////// Debugging //////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

void writeUUEDataTofile(uint8_t UUE_Encoded_String[], int UUE_Encoded_String_Index)
{
	uint8_t UnixFilePermissions[] = "0777";

	UUEDataFile = open_file ("uueFile.uue", "w" );
	
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
		for (int characterIndex = 0; characterIndex < UUE_Encoded_String_Index; characterIndex+=0)
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

void writeHexDataTofile(struct Data data_local)
{
	
	int totalDataIndex = 0;
	hexDataFile = open_file ("hexFile.hex", "w" );
	if (hexDataFile == NULL) {
		printf("I couldn't open hexFile.hex for writing.\n");
		exit(0);
	}
	else{
		for (int char_count_index = 0; totalDataIndex < data_local.HEX_array_size; char_count_index+=0)
		{
			for (int line_index = 0; line_index < 16; ++line_index)
			{
				fprintf(hexDataFile, "%02X", data_local.HEX_array[totalDataIndex]);
				totalDataIndex++;
				// If we reach the end-of-data, exit loops.
				if (totalDataIndex == data_local.HEX_array_size){break;}
			}
			fprintf(hexDataFile, "\n");		
		}
		
	}
	
}

//Open file for reading, function.
FILE *open_file ( uint8_t *file, uint8_t *mode )
{
  FILE *fileOpen = fopen ( file, mode );

  if ( fileOpen == NULL ) {
    perror ( "Unable to open file" );
    exit (EXIT_FAILURE);
  }

  return fileOpen;
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

int tx_size_unsigned(uint8_t * string)
{
	// Used to size data array's for 
	int count_char_in_string = 0;
	while(string[count_char_in_string] != '\n'){count_char_in_string++;}
	count_char_in_string++;

	return count_char_in_string;
}

int tx_size_signed(char * string)
{
	// Used to size data array's for 
	int count_char_in_string = 0;
	while(string[count_char_in_string] != '\n'){count_char_in_string++;}
	count_char_in_string++;

	return count_char_in_string;
}

// Debugging and Print Handling.
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
	printf("**** Vorpal Hoff v.133                                               *****\n");
	printf("**** In memory of Hoff70                                             *****\n");
	printf("**** ALABTU!                                                         *****\n");
	printf("****                                           Thanks to Bdk6        *****\n");
	printf("****                                       His help was a necessity. *****\n");
	printf("**************************************************************************\n");
	printf("\n");
	clearConsole();
}

int sectors_needed(int hex_data_array_size)
{
	int sectors_needed = 1;
	while(sectors_needed * 4096 < hex_data_array_size)
	{
		sectors_needed++;
	}	
	return sectors_needed;
}

int prepare_sectors(int sectors)
{
	uint8_t sectors_needed_string[128];
	snprintf(sectors_needed_string, sizeof(sectors_needed_string), "P %i %i\n", sectors-1, sectors-1);
	tx_chars(sectors_needed_string, tx_size_signed(sectors_needed_string), PRINT, 0);
	tx_chars("\n", sizeof("\n"), PRINT, 0);
	Sleep(300);
	rx(PARSE, PRINT);	

	return 0;	
}

struct write validity_checksum(struct write write_local, struct Data data_local)
{
	// 1. Read vector table (bytes) 0-6
	// 2. Add the 32 bit words together.
	// 3. Take the sum and create two's complement.
	// 4. Insert this in the 8th word.

	uint32_t chck_sum_array[8];
	uint32_t check_sum = 0;
	uint32_t word = 0;
	uint32_t validity_sum = 0;

	// Little Endian
	for (int i = 0; i < 28; i+=4)
	{
		chck_sum_array[i/4] = data_local.HEX_array[i] + (data_local.HEX_array[i+1] << 8) + (data_local.HEX_array[i+2] << 16) + (data_local.HEX_array[i+3] << 24);
	}

	for (int i = 0; i < 7; ++i)
	{
		check_sum += chck_sum_array[i];
	}

	check_sum = (~check_sum) + 1;
	data_local.HEX_array[28] = check_sum & 0xff;
	data_local.HEX_array[29] = (check_sum >>8 ) & 0xff;
	data_local.HEX_array[30] = (check_sum>>16) & 0xff;
	data_local.HEX_array[31] = (check_sum>>24) & 0xff;

	for (int i = 0; i < 31; i+=4)
	{
		word = data_local.HEX_array[i] + (data_local.HEX_array[i+1] << 8) + (data_local.HEX_array[i+2] << 16) + (data_local.HEX_array[i+3] << 24);
		validity_sum += word;
	}

	printf("Validity sum: %"PRId32"\n", validity_sum);
	
	return write_local;
}

// Write to RAM.
struct write write_page_to_ram(struct write write_local, struct Data data_local)
{
	// 1. Convert RAM address_A from hex to decimal, then from decimal to ASCII.
	// 2. Create intent-to-write-to-ram string: "W 268435456 512\n"
	// 3. Send intent-to-write string.
	// 4. Send chunk_A of data: "DATA\n"
	// 5. Send checksum: "Chk_sum\n"
	// 6. Read response from LPC.
	// 8. Repeat write if necessary.
	// 9. Return true if successful (combine step 9?)
	
	// Locals for creating intent_to_write string.
	uint8_t address_as_string[9];
	uint32_t dec_ram_address = 0;
	uint8_t dec_address_as_string[32];
	uint8_t intent_to_write_to_ram_string[512];
	uint8_t checksum_as_string[64];
	
	// Used to determine if write operation succeeded.
	int successful = 0;

	// Used to slow txString if write failed.
	int slow_down = 0;

	int bytes_left_to_write = (data_local.HEX_array_size - write_local.bytes_written);
	//printf("bytes_left_to_write %i\n", bytes_left_to_write);

	static int write_success = 0, write_attempt = 0;
	float success_ratio = 0;

	// ISP uses RAM from 0x1000 017C to 0x1000 17F
	write_local.ram_address = 0x1000017C;

	/////// CHUNK A ////////

	// 1. Convert RAM address_A from hex to decimal, then from decimal to ASCII.
	convert_32_hex_address_to_string(write_local.ram_address, address_as_string);
	dec_ram_address = strtol(address_as_string, NULL, 16);
	snprintf(dec_address_as_string, sizeof(dec_address_as_string), "%d", dec_ram_address);

	// 2. Create intent-to-write-to-ram string: "W 268435836 512\n"
	//snprintf(intent_to_write_to_ram_string, sizeof(intent_to_write_to_ram_string), "W %s %i\n", dec_address_as_string, 128);
	snprintf(intent_to_write_to_ram_string, sizeof(intent_to_write_to_ram_string), "W 268435840 %i\n", 128);

	// Loops until succeeded.
	while(successful < 3){
		// 3. Send intent-to-write string.
		tx_chars(intent_to_write_to_ram_string, tx_size_signed(intent_to_write_to_ram_string), PRINT, 0);
		tx_chars("\n", sizeof("\n"), NO_PRINT, 0);
		Sleep(100);
		rx(PARSE, PRINT);

		// 4. Send chunk_A of data: "DATA\n"
		tx_data(write_local.UUE_chunk_A, write_local.UUE_chunk_A_UUE_char_count, PRINT, slow_down);
		Sleep(300);	
		tx_chars("\n", sizeof("\n"), NO_PRINT, 0);

		// 5. Send checksum: "Chk_sum\n"
		snprintf(checksum_as_string, 10, "%i\n", write_local.UUE_chunk_A_check_sum);
		tx_chars(checksum_as_string, tx_size_signed(checksum_as_string), PRINT, 0);
		tx_chars("\n", sizeof("\n"), PRINT, 0);
		Sleep(140);
	
		// 6. Read response from LPC.
		successful = rx(PARSE, PRINT);
		write_attempt++;	

		slow_down += 1;
		// 8. Repeat write if necessary.
	} // Loop if fail.
	if (successful == 3)
	{
		// No need to slow down.
		slow_down = 0;

		//Success counter.
		write_success++;

		// Reset success counter for B
		successful = 0;
				
		// We show chunk A has been written.
		write_local.bytes_written += write_local.bytes_loaded_A;
	}

	//printf("Bytes written: %i\n", write_local.bytes_written);


	// Increament RAM address by 128.	
	write_local.ram_address += 128;
	/////// CHUNK B ////////

	// 1. Convert RAM address_A from hex to decimal, then from decimal to ASCII.
	convert_32_hex_address_to_string(write_local.ram_address, address_as_string);
	dec_ram_address = strtol(address_as_string, NULL, 16);
	snprintf(dec_address_as_string, sizeof(dec_address_as_string), "%d", dec_ram_address);

	// 2. Create intent-to-write-to-ram string: "W 268435456 512\n"
	//snprintf(intent_to_write_to_ram_string, sizeof(intent_to_write_to_ram_string), "W %s %i\n", dec_address_as_string, 128);
	snprintf(intent_to_write_to_ram_string, sizeof(intent_to_write_to_ram_string), "W 268435968 %i\n", 128);

	// Loops until succeeded.
	while(successful < 3){
		// 3. Send intent-to-write string.
		tx_chars(intent_to_write_to_ram_string, tx_size_signed(intent_to_write_to_ram_string), PRINT, 0);
		tx_chars("\n", sizeof("\n"), NO_PRINT, 0);
		Sleep(100);
		rx(PARSE, PRINT);

		// 4. Send chunk_A of data: "DATA\n"
		tx_data(write_local.UUE_chunk_B, write_local.UUE_chunk_B_UUE_char_count, PRINT, slow_down);
		Sleep(200);	
		tx_chars("\n", sizeof("\n"), PRINT, 0);

		// 5. Send checksum: "Chk_sum\n"
		snprintf(checksum_as_string, 10, "%i\n", write_local.UUE_chunk_B_check_sum);
		tx_chars(checksum_as_string, tx_size_signed(checksum_as_string), NO_PRINT, 0);
		tx_chars("\n", sizeof("\n"), PRINT, 0);
		Sleep(110);
	
		// 6. Read response from LPC.
		successful = rx(PARSE, PRINT);
		write_attempt++;
		// 8. Repeat write if necessary.
	} // Loop if fail.
	if (successful == 3)
	{
		// No need to slow down.
		slow_down = 0;

		//Success counter.
		write_success++;

		// Reset success counter for B
		successful = 0;

		// We show chunk A has been written.
		write_local.bytes_written += write_local.bytes_loaded_B;
	}

	// Update how many bytes we have left.
	bytes_left_to_write = (data_local.HEX_array_size - write_local.bytes_written);
	//printf("bytes_left_to_write %i\n", bytes_left_to_write);
	
	//printf("Bytes written: %i\n", write_local.bytes_written);
	
	// Calculates the success rate of write()
	success_ratio = ((float)write_success / write_attempt) * 100;
	printf("Write to RAM success: %%%.2f\n", success_ratio);

	return write_local;
}


struct write prepare_page_to_write(struct write write_local, struct Data data_local)
{
	
	// 0. Load arrays with FF.
	// 1. Load hex data into array A & get #bytes.
	// 2. UUEncode the chunk & get UUE char count.
	// 3. Calculate chunk checksum.
	// 4. Repeat 1-3 for B.

	uint8_t HEX_chunkA_array_buf[128];
	uint8_t HEX_chunkB_array_buf[128];

	// 0. Load arrays with FF.
	for (int i = 0; i < 128; ++i)
	{
		HEX_chunkA_array_buf[i] = 0xFF;
		HEX_chunkB_array_buf[i] = 0xFF;	
	}

	// 1. Load hex data into array A & get #bytes.
	
	// CHUNK A
	for (int i = 0; i < 128; ++i)
	{
		if (write_local.bytes_loaded_A < data_local.HEX_array_size)
		{
			HEX_chunkA_array_buf[i] = data_local.HEX_array[(128*write_local.chunk_index)+(write_local.sector_index * 4096)+i];	
		}
		
	//	printf("X:%i\n", (128*write_local.chunk_index)+(write_local.sector_index * 4096)+i);
		write_local.bytes_loaded_A++;
		//printf("%02X ", HEX_chunkA_array_buf[i]);
	}
	write_local.chunk_index++;
	//printf("\n\n");

	// CHUNK B
	for (int i = 0; i < 128; ++i)
	{
		if (write_local.bytes_loaded_B < data_local.HEX_array_size)
		{
			HEX_chunkB_array_buf[i] = data_local.HEX_array[(128*write_local.chunk_index)+(write_local.sector_index * 4096)+i];	
		}
		write_local.bytes_loaded_B++;
		//printf("%i\n", ((128*write_local.chunk_index)+(write_local.sector_index * 4096)+i));
		//printf("%02X ", HEX_chunkB_array_buf[i]);
	}
	write_local.chunk_index++;

	//printf("\n\n");
	//printf("bytes loaded: %i\n", write_local.bytes_loaded);
	

	// 2. UUEencode the chunk & get UUE char count.
	write_local.UUE_chunk_A_UUE_char_count = UUEncode(write_local.UUE_chunk_A, HEX_chunkA_array_buf, 128);
	write_local.UUE_chunk_B_UUE_char_count = UUEncode(write_local.UUE_chunk_B, HEX_chunkB_array_buf, 128);

	// 3. Calculate chunk checksum.
	write_local.UUE_chunk_A_check_sum =	check_sum(HEX_chunkA_array_buf, 128);
	write_local.UUE_chunk_B_check_sum =	check_sum(HEX_chunkB_array_buf, 128);

	// Print encoded chunks A & B.
	//printf("\n\n");
	//for (int i = 0; i < 256; ++i)
	//{
	//	printf("%C", write_local.UUE_chunk_A[i]);
	//}
	
	//printf("\nchar count A %i\n", write_local.UUE_chunk_A_UUE_char_count);
	//printf("\ncheck sum A %i\n", write_local.UUE_chunk_A_check_sum);
	//printf("\n\n");

	//for (int i = 0; i < 256; ++i)
	//{
	//	printf("%C", write_local.UUE_chunk_B[i]);
	//}
	//printf("\nchar count B %i\n", write_local.UUE_chunk_B_UUE_char_count);
	//printf("\ncheck sum B %i\n", write_local.UUE_chunk_B_check_sum);

	return write_local;
}

struct write erase_chip(struct write write_local)
{
		uint8_t sectors_needed_string[128];
		snprintf(sectors_needed_string, sizeof(sectors_needed_string), "P 0 7\n");
		tx_chars(sectors_needed_string, tx_size_signed(sectors_needed_string), PRINT, 0);
		tx_chars("\n", sizeof("\n"), PRINT, 0);
		rx(PARSE, PRINT);
		Sleep(300);
		
		rx(PARSE, PRINT);
		tx_chars("E 0 1 2 3 4 5 6 7", sizeof("E 0 1 2 3 4 5 6 7"), PRINT, 0);
		tx_chars("\n", sizeof("\n"), PRINT, 0);
		Sleep(200);
		rx(PARSE, PRINT);

		return write_local;
}

struct write ram_to_flash(struct write write_local, struct Data data_local)
{
	// Locals for creating intent_to_write string.
	uint8_t address_as_string[9];
	uint32_t dec_flash_address = 0;
	uint8_t flash_address_as_string[32];
	uint8_t intent_to_write_to_flash_string[512];

	// Used to determine sector ( 32 * 128 = 4096)
	static int page_index;
	
	printf("write_local.Flash_address %02X\n", write_local.Flash_address);
	// 0. Point to current sector.
	if (page_index == 16)
	{

		write_local.sector_index--;
		if(write_local.sector_index < 0)
		{
			write_local.sector_index = 0;
		}
		write_local.chunk_index = 0;
		write_local.Flash_address = (write_local.sector_index * 4096);
		printf("%02X\n", write_local.Flash_address);
		if (write_local.Flash_address < 0)
		{
			write_local.Flash_address = 0;
		}
		page_index = 0;
	}

	//printf("chunk indxex %i\n", page_index);
	//printf("sector_to_write %i\n", write_local.sector_to_write);
	
	// 0.5 Preare those sectors.
	prepare_sectors(write_local.sector_index+1);

	// 1. Convert RAM address_A from hex to decimal, then from decimal to ASCII.
	convert_32_hex_address_to_string(write_local.Flash_address, address_as_string);
	dec_flash_address = strtol(address_as_string, NULL, 16);
	snprintf(flash_address_as_string, sizeof(flash_address_as_string), "%d", dec_flash_address);

	// 2. Create intent-to-write-to-ram string: "W 268435456 512\n"
	snprintf(intent_to_write_to_flash_string, sizeof(intent_to_write_to_flash_string), "C %s 268435840 %i\n", flash_address_as_string, 256);

	tx_chars(intent_to_write_to_flash_string, tx_size_signed(intent_to_write_to_flash_string), PRINT, 0);
	tx_chars("\n", sizeof("\n"), PRINT, 0);
	Sleep(300);
	rx(PARSE, PRINT);

	//printf("%s\n", intent_to_write_to_flash_string);
	//printf("\n\n\n");
	//printf("write_local.Flash_address %02X\n", write_local.Flash_address);
	write_local.Flash_address += 256;

	page_index++;

	return write_local;
}


void convert_32_hex_address_to_string(uint32_t address, uint8_t * address_as_string)
{
	// 1. Divide 32 bit int into nybbles.
	// 2. Convert nybble into character.
	// 3. Place characters into string.
	int char_index = 0;
	uint8_t buf_nybble;
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

// TIMERS.
float timer()
{	
	static clock_t start; 
	double time_elapsed_in_seconds;
	clock_t end = clock();
	time_elapsed_in_seconds = (end - start)/(double)CLOCKS_PER_SEC;
	start = clock();
	return time_elapsed_in_seconds;
}

void usleep(__int64 usec) 
{ 
    HANDLE timer; 
    LARGE_INTEGER ft; 

    ft.QuadPart = -(10*usec); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL); 
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0); 
    WaitForSingleObject(timer, INFINITE); 
    CloseHandle(timer); 
}
