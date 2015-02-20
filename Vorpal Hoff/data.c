#include "data.h"


extern uint8_t ParsedRxBuffer[2048];
extern uint8_t RawRxBuffer[2048];
extern int command_response_code;
extern FT_HANDLE handle;
extern FT_STATUS FT_status;
extern DWORD EventDWord;
extern DWORD TxBytes;
extern DWORD BytesWritten;
extern DWORD RxBytes;
extern DWORD BytesReceived;
extern uint8_t RawRxBuffer[2048];
extern uint8_t ParsedRxBuffer[2048];

//File to be loaded.	
extern FILE *fileIn;
extern FILE *hexDataFile;
extern FILE *UUEDataFile;
extern FILE *debug;

//Total bytesRead.
extern int totalCharsRead;
//Reading characters from a file.
extern uint8_t charToPut;

// Data Handling
struct Data hex_file_to_array(struct Data data_local, int file_size)
{
	
	//To hold file hex values.
	uint8_t fhex_byte_count[MAX_SIZE_16];
	uint8_t fhex_address1[MAX_SIZE_16];
	uint8_t fhex_address2[MAX_SIZE_16];
	int combined_address = 0;
	uint8_t fhex_record_type[MAX_SIZE_16];
	uint8_t fhex_check_sum[MAX_SIZE_16];

	//To count through all characters in the file.
	int file_char_index = 0;
	int hex_data_index = 0;
	int hex_line_index = 0;
	
	//Holds line count.	
	int chars_this_line = 0;

	//Loop through each character until EOF.
	while(totalCharsRead  < file_size){
		
		//BYTE COUNT
		fhex_byte_count[file_char_index] = readByte();
		
		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		fhex_address1[file_char_index] = readByte();
		
		//ADDRESS2
		fhex_address2[file_char_index] = readByte();
		
		//RECORD TYPE
		fhex_record_type[file_char_index] = readByte();	

		if (fhex_record_type[file_char_index] == 0)
		{
			combined_address = fhex_address1[file_char_index];
			combined_address <<= 8;
			combined_address |= fhex_address2[file_char_index];
			combined_address =  combined_address/16;
		}
		
		//Throws the byte count (data bytes in this line) into an integer.
		chars_this_line = fhex_byte_count[file_char_index];

		//////// DATA ///////////////////
		// We only want data. Discard other data types.
		if (fhex_record_type[file_char_index] == 0)
		{
			while (hex_data_index < chars_this_line && totalCharsRead < file_size && chars_this_line != 0x00)
			{
				//Store the completed hex value in the char array.
				data_local.HEX_array[data_local.HEX_array_size] = readByte();
				
				//Index for data.
				data_local.HEX_array_size++;	
				hex_data_index++;			
			}

		//Reset loop index for characters on this line.
		hex_data_index = 0;
		}
		
		//////// CHECK SUM //////////////
		if (charToPut != 0xFF){
			fhex_check_sum[file_char_index] = readByte();
		}

		hex_line_index++;
		file_char_index++;
	}

	// Assure # of bytes are divisble by 4.
	while(!(data_local.HEX_array_size % 4 == 0))
	{
		data_local.HEX_array[data_local.HEX_array_size] = ' ';
		data_local.HEX_array_size++;
	}

	return data_local;
}

int UUEncode(uint8_t * UUE_data_array, uint8_t * hex_data_array, int hex_data_array_size)
{
	// 1. Add char for characters per line.
	// 2. Load 3 bytes into an array.
	// 3. Encode array.
	// 4. Add padding.
	// 5. Replace ' ' with '''
	// 6. Return UUE data array (implicit) and size.
	uint8_t byte_to_encode[3];
	uint8_t uue_char[4];

	int UUE_encoded_string_index = 0;
	int uue_length_char_index = 45;
	int padded_index = 0;
	int bytes_left = 0;

	for (int i = 0; i < hex_data_array_size; ++i)
	{
		//printf("%02X ", hex_data_array[i]);
	}
	
	// 1. Add char for characters per line.
	if(hex_data_array_size < 45)
	{
		 UUE_data_array[UUE_encoded_string_index] = ((hex_data_array_size << 2) >> 2) + ' ';
	}
	else
	{
		UUE_data_array[UUE_encoded_string_index] = 'M';
	}

	UUE_encoded_string_index++;

	// Encode loop.
	for (int hex_data_array_index = 0; hex_data_array_index < hex_data_array_size; hex_data_array_index)
	{
		// 2. Load 3 bytes into an array.
		for (int i = 0; i < 3; ++i)
		{
			// Load bytes into array
			if (hex_data_array_index < hex_data_array_size)
			{
				byte_to_encode[i] = hex_data_array[hex_data_array_index];
				hex_data_array_index++;
			}
			else
			{
				// 4. Add padding.
				byte_to_encode[i] = 0;
				padded_index++;
			}
			uue_length_char_index--;
		}

		// 3. Encode array.
		uue_char[0] = ((byte_to_encode[0] >> 2) & 0x3f);
		uue_char[1] = (((byte_to_encode[0] << 4) | ((byte_to_encode[1] >> 4) & 0x0f)) & 0x3f);
		uue_char[2] = (((byte_to_encode[1] << 2) | ((byte_to_encode[2] >> 6) & 0x03)) & 0x3f);
		uue_char[3] = (byte_to_encode[2] & 0x3f);

		for (int i = 0; i < 4; i++)
		{
			// 5. Replace ' ' with '''
			if (uue_char[i] == 0x00)
			{
				UUE_data_array[UUE_encoded_string_index] = 0x60;
			}
			else
			{
				UUE_data_array[UUE_encoded_string_index] = (uue_char[i] + ' ');
			}

			UUE_encoded_string_index++;
		}

		// Data bytes left.
		bytes_left = (hex_data_array_size - hex_data_array_index);

		if (uue_length_char_index == 0 && bytes_left > 0)
		{
			// NOTE: Could be simplified to include first char
			// and additional characters, using a positive index.
			// 1. Add char for characters per line.
			UUE_data_array[UUE_encoded_string_index] = '\n';
			UUE_encoded_string_index++;

			if(bytes_left < 45)
			{
				// Find how many characters are left.
				UUE_data_array[UUE_encoded_string_index] = ((bytes_left & 0x3f) + ' ');
			}
			else
			{
				UUE_data_array[UUE_encoded_string_index] = 'M';
			}	
			UUE_encoded_string_index++;
			uue_length_char_index = 45;
		}

	} // End UUE loop	
	UUE_data_array[UUE_encoded_string_index] = '\n';

	// Return count of UUE chars.
	return UUE_encoded_string_index;
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

int check_sum(uint8_t * hex_data_array, int hex_data_array_size)
{
	int check_sum = 0;
	int char_index = 0;

	while(char_index < hex_data_array_size)
	{
		check_sum += hex_data_array[char_index];
		char_index++;
	}
	return check_sum;
}

uint8_t readByte(){
	//Holds combined nibbles.
	char ASCII_hexvalue[3];
	char hex_hexvalue;
	char * pEnd;

	//Put first nibble in.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	ASCII_hexvalue[0] = (uint8_t)charToPut;
	
	//Put second nibble in.
	charToPut = fgetc (fileIn);
	clearSpecChar();
	ASCII_hexvalue[1] = (uint8_t)charToPut;

	// Increase counter for total characters read from file.
	totalCharsRead+=2;

	// Convert the hex string to base 16.
	hex_hexvalue = strtol(ASCII_hexvalue, &pEnd, 16);
	
	return hex_hexvalue;	
}

//Copied in from lpc21isp.c
uint8_t Ascii2Hex(uint8_t c)
{
	if (c >= '0' && c <= '9')
	{
		return (uint8_t)(c - '0');
	}
	if (c >= 'A' && c <= 'F')
	{
		return (uint8_t)(c - 'A' + 10);
	}
	if (c >= 'a' && c <= 'f')
	{
        return (uint8_t)(c - 'A' + 10);
	}
	//printf("\n !!! Bad Character: %02X in file at totalCharsRead=%d !!!\n\n", c, totalCharsRead);
	return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
} 
