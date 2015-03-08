#include "data.h"

/*------------------------------------------------- DATA.c ------------
|  Copyright (c) 2015 C. Thomas Brittain
|							aka, Ladvien
*-------------------------------------------------------------------*/

int file_sizer(FILE *file_to_size)
{
	int fileSize = 0;
	while((fgetc (file_to_size)) != EOF)
	{
		fileSize++;
	}
	rewind(file_to_size);
	return fileSize;
}

void decode_three(uint8_t * ret, char c0, char c1, char c2, char c3)
{
	uint8_t b0 = (uint8_t)c0;
	uint8_t b1 = (uint8_t)c1;
	uint8_t b2 = (uint8_t)c2;
	uint8_t b3 = (uint8_t)c3;

	if(b0 > 0x20 && b1 > 0x20 && b2 > 0x20 && b3 > 0x20 &&
		b0 < 0x61 && b1 < 0x61 && b2 < 0x61 && b3 < 0x61)
	{
		uint32_t word;
		word = (b0 - 0x20 & 0x3f) << 18;
		word |= (b1 - 0x20 & 0x3f) << 12;
		word |= (b2 - 0x20 & 0x3f) << 6;
		word |= (b3 - 0x20 & 0x3f);

	    ret[0] = (uint8_t)((word >> 16) & 0xff);
		ret[1] = (uint8_t)((word >> 8) & 0xff);
		ret[2] = (uint8_t)((word) & 0xff);

	 	printf("%02X %02X %02X\n", ret[0], ret[1], ret[2]);     
 	}
}

// Data Handling
int hex_file_to_array(FILE * file, uint8_t hex_data[], int file_size)
{
	// 1. Get line count.
	// 2. Read a line. From ':' to '\n'
	// 3. Parse a line. 
	//   Repeat for all lines.

	int total_chars_read = 0;

	// Data per line.
	uint8_t line_of_data[32];
	long int combined_address[4096];

	// Indexes
	int hex_line_index = 0;
	// Line dimesions
	int chars_this_line = 0;

	// How many lines in the hexfile?
	int hex_lines_in_file = 0;

	int bytes_this_line[4096];

	hex_lines_in_file = hex_file_line_count(file);

	int line_index = 0;
	int byte_index = 0;
	bool what = true;
	while(line_index < hex_lines_in_file)
	{
		what = read_line_from_hex_file(file, line_of_data, &combined_address[line_index], &bytes_this_line[line_index]);
		if (!what)
		{
			printf("Line#: %i. Dude, that's not data!\n", line_index);

			what = true;
		}
		while(byte_index < bytes_this_line[line_index])
		{
			hex_data[combined_address[line_index] + byte_index] = line_of_data[byte_index];
			line_of_data[byte_index] = '\0';
			byte_index++;
		}
		byte_index = 0;
		line_index++;
	}

	int k = 0;
	int j = 0;
	int printed_bytes = 0;
	while (k < hex_lines_in_file)
	{
		while(j < bytes_this_line[k])
		{
			printf("%02X ", hex_data[j+(printed_bytes)]);
			j++;
		}
		printed_bytes += bytes_this_line[k];
		j=0;
		printf("\n");
		k++;
	}
	
	return total_chars_read;
} // End hex_file_to_array

bool read_line_from_hex_file(FILE * file, uint8_t line_of_data[], long int * combined_address, int * bytes_this_line)
{
		int data_index = 0;
		uint8_t char_to_put;
		int total_chars_read = 0;

		//To hold file hex values.
		uint8_t byte_count;
		uint8_t datum_address1;
		uint8_t datum_address2;
		uint8_t datum_record_type;
		uint8_t datum_check_sum;

		//BYTE COUNT
		byte_count = read_byte_from_file(file, &char_to_put, &total_chars_read);

		// No need to read, if no data.
		if (byte_count == 0){return false;}

		//ADDRESS1 //Will create an 8 bit shift. --Bdk6's
		datum_address1 = read_byte_from_file(file, &char_to_put, &total_chars_read);

		//ADDRESS2
		datum_address2 = read_byte_from_file(file, &char_to_put, &total_chars_read);

		//RECORD TYPE
		datum_record_type = read_byte_from_file(file, &char_to_put, &total_chars_read);		

		// No need to read, if not data.
		if (datum_record_type != 0){return false;}

		*combined_address = ((uint16_t)datum_address1 << 8) | datum_address2;

		// DATA
		while(data_index < byte_count)
		{
			line_of_data[data_index] = read_byte_from_file(file, &char_to_put, &total_chars_read);
			data_index++;
		}			
		*bytes_this_line = data_index;

		// CHECKSUM
		datum_check_sum = read_byte_from_file(file, &char_to_put, &total_chars_read);

		return true;
}

int hex_file_line_count(FILE * file_to_count)
{
	int line_count = 0;
	char got_char;

	while(got_char != EOF)
	{
		got_char = fgetc(file_to_count);
		if (got_char == ':'){line_count++;}
	}
	rewind(file_to_count);
	return line_count;
}



uint8_t read_byte_from_file(FILE * file, uint8_t * char_to_put, int * total_chars_read)
{
	//Holds combined nibbles.
	uint8_t hexValue;
	//Get first nibble.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put first nibble in.
	hexValue = (Ascii2Hex(*char_to_put));
	//Slide the nibble.
	hexValue = ((hexValue << 4) & 0xF0);
	//Put second nibble in.
	*char_to_put = fgetc (file);
	clear_special_char(file, char_to_put, total_chars_read);
	//Put the nibbles together.
	hexValue |= (Ascii2Hex(*char_to_put));
	//Return the byte.
	*total_chars_read += 2;

	return hexValue;
}

void clear_special_char(FILE * file, uint8_t * char_to_put, int * total_chars_read)
{
	//Removes CR, LF, ':'  --Bdk6's
	while (*char_to_put == '\n' || *char_to_put == '\r' || *char_to_put ==':'){
		(*char_to_put = fgetc (file));
		*total_chars_read++;
	}
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
	//printf("\n !!! Bad Character: %02X in file at total_chars_read=%d !!!\n\n", c, total_chars_read);
	return 0;  // this "return" will never be reached, but some compilers give a warning if it is not present
}

int make_array_multiple_of_four(uint8_t * hex_data, int hex_array_size)
{
	// Assure # of bytes are divisble by 4.
	while(!(hex_array_size % 4 == 0))
	{
		hex_data[hex_array_size] = ' ';
		hex_array_size++;
	}	
	return hex_array_size;
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

