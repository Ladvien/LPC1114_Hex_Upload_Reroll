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

	 	printf("%02X %02X %02X ", ret[0], ret[1], ret[2]);     
 	}
}

void decode_UUE_line(char UUE_line[], char decoded_HEX_array[])
{

	// ADD WAY TO INDEX ARRAY STATICALLY.

	int UUE_char_count = 0;
	int bytes_this_line = (int)UUE_line[UUE_char_count]-32;

	printf("%i\n", bytes_this_line);

	while(bytes_this_line < UUE_char_count){
		decode_three(decoded_HEX_array, UUE_line[UUE_char_count], UUE_line[UUE_char_count+1], UUE_line[UUE_char_count+2], UUE_line[UUE_char_count+3]);
		UUE_char_count+=4;
	}
}


int get_UUE_line_from_array(char UUE_line[], char uue_data[], bool reset){

	static int index = 0;
	if(reset){index = 0;}
	if (uue_data[index] == '\n')
	{
		index++;
	}
	int bytes_this_line = (int)(((uue_data[index]-32)*1.333)+.5);
	bytes_this_line+=1;

	int instance_index = 0;

	while(instance_index < bytes_this_line){
		UUE_line[instance_index] = uue_data[index];
		instance_index++;
		index++;
	}
	index++;
	return bytes_this_line;
}

int get_UUE_file_into_array(FILE * file, char uue_data[]){
	int file_index = 0;
	int file_size = 0;

	file_size = file_sizer(file);

	int new_line_counter = 0;

	while(file_index < file_size){
		uue_data[file_index] = fgetc (file);
		printf("%c", uue_data[file_index]);
		if(uue_data[file_index] == '\n'){
			new_line_counter++;
			printf("Line #%i: ", new_line_counter);
		}
		file_index++;			
	}

	// the output file has an extra LF at the end.  We subtract it.
	return new_line_counter;
}

// Data Handling
int hex_file_to_array(FILE * file, uint8_t hex_data[])
{
	// 1. Get line count.
	// 2. Read a line. From ':' to '\n'
	// 3. Parse a line. 
	//   Repeat for all lines.

	// Data per line.
	uint8_t line_of_data[32];
	long int combined_address[4096];

	// Indices and counters
	int hex_line_index = 0;
	int chars_this_line = 0;
	int total_chars_read = 0;
	// How many lines in the hexfile?
	int hex_lines_in_file = 0;
	int bytes_this_line[4096];

	// Let's count how many lines are in this file.
	hex_lines_in_file = hex_file_line_count(file);

	// Indices for parsing.
	int line_index = 0;
	int byte_index = 0;
	bool read_line_ok = false;
	int total_bytes_found = 0;

	int data_count = 0;
	while (data_count < 32768){
		//hex_data[data_count] = 0xFF;
		data_count++;
	}

	// Parse all lines in file.
	while(line_index < hex_lines_in_file)
	{
		read_line_ok = read_line_from_hex_file(file, line_of_data, &combined_address[line_index], &bytes_this_line[line_index]);
		if (!read_line_ok)
		{
			printf("Line#: %i. Dude, that's not data!\n", line_index);
			read_line_ok = true;
		}
		while(byte_index < bytes_this_line[line_index])
		{
			hex_data[combined_address[line_index] + byte_index] = line_of_data[byte_index];
			line_of_data[byte_index] = '\0';
			byte_index++;
			total_bytes_found++;
		}
		byte_index = 0;
		line_index++;
	}

	// Print out parsed data.
	int k = 0;
	int j = 0;
	int printed_bytes = 0;
	while (k < hex_lines_in_file)
	{
		printf("%i: ", k+1);
		//while(j < bytes_this_line[k])
		while(j < 16)
		{
			printf("%02X ", hex_data[j+(printed_bytes)]);
			j++;
		}
		printed_bytes += bytes_this_line[k];
		j=0;
		printf("\n");
		k++;
	}
	
	return total_bytes_found;
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

	int UUEncoded_array_index = 0;
	int uue_length_char_index = 45;
	int padded_index = 0;
	int bytes_left = 0;

	// 1. Add char for characters per line.
	if(hex_data_array_size < 45)
	{
		 UUE_data_array[UUEncoded_array_index] = ((hex_data_array_size & 0x3f) + ' ');
	}
	else
	{
		UUE_data_array[UUEncoded_array_index] = 'M';
	}

	UUEncoded_array_index++;

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
				UUE_data_array[UUEncoded_array_index] = 0x60;
			}
			else
			{
				UUE_data_array[UUEncoded_array_index] = (uue_char[i] + ' ');
			}

			UUEncoded_array_index++;
		}

		// Data bytes left.
		bytes_left = (hex_data_array_size - hex_data_array_index);

		if (uue_length_char_index == 0 && bytes_left > 0)
		{
			// NOTE: Could be simplified to include first char
			// and additional characters, using a positive index.
			// 1. Add char for characters per line.
			UUE_data_array[UUEncoded_array_index] = '\n';
			UUEncoded_array_index++;

			if(bytes_left < 45)
			{
				// Find how many characters are left.
				UUE_data_array[UUEncoded_array_index] = ((bytes_left & 0x3f) + ' ');
			}
			else
			{
				UUE_data_array[UUEncoded_array_index] = 'M';
			}	
			UUEncoded_array_index++;
			uue_length_char_index = 45;
		}

	} // End UUE loop	
	UUE_data_array[UUEncoded_array_index] = '\n';

	// 6. Return UUE data array (implicit) and size.
	return UUEncoded_array_index;
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

