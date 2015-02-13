struct write validity_checksum(struct write write_local, struct Data data_local)
{
	// 1. Read vector table (bytes) 0-6
	// 2. Add the 32 bit words together.
	// 3. Take the sum and create two's complement.
	// 4. Insert this in the 8th word.

	uint32_t chck_sum_array[8];
	uint8_t check_sum_array_bytes[4];
	uint32_t check_sum_test = 0;
	uint32_t check_sum = 0;

	// Little Endian
	for (int i = 0; i < 28; i+=4)
	{
		chck_sum_array[i/4] = data_local.HEX_array[i] + (data_local.HEX_array[i+1] << 8) + (data_local.HEX_array[i+2] << 16) + (data_local.HEX_array[i+3] << 24);
	}

	// Big Endian
	//for (int i = 0; i < 28; i+=4)
	//{
	//	chck_sum_array[i/4] = (data_local.HEX_array[i] << 24)+ (data_local.HEX_array[i+1] << 16) + (data_local.HEX_array[i+2] << 8) + data_local.HEX_array[i+3];
	//	printf("HERE %i\n", i);
	//}

	
	for (int i = 0; i < 7; ++i)
	{
		check_sum += chck_sum_array[i];
	}
	check_sum = (~check_sum) + 1;

	printf("CHECK SUM !!! %02X\n", check_sum);

	check_sum_array_bytes[0] = (check_sum & 0xFF); // LSB
	check_sum_array_bytes[1] = ((check_sum >> 8) & 0xFF);
	check_sum_array_bytes[2] = ((check_sum >> 16) & 0xFF);
	check_sum_array_bytes[3] = ((check_sum >> 24) & 0xFF);
	
	printf("OTHER !!! %02X %02X %02X %02X\n", check_sum_array_bytes[0], check_sum_array_bytes[1], check_sum_array_bytes[2], check_sum_array_bytes[3]);

	chck_sum_array[7] = (check_sum_array_bytes[3] & 0xFF); // LSB
	chck_sum_array[7] = ((chck_sum_array[7] << 8) |  check_sum_array_bytes[2]);
	chck_sum_array[7] = ((chck_sum_array[7] << 8) |  check_sum_array_bytes[1]);
	chck_sum_array[7] = ((chck_sum_array[7] << 8) |  check_sum_array_bytes[0]);

	printf("FINAL CHECK! !!! %02X\n", chck_sum_array[7]);

	//
	data_local.HEX_array[31] = check_sum_array_bytes[0];
	data_local.HEX_array[30] = check_sum_array_bytes[1];
	data_local.HEX_array[29] = check_sum_array_bytes[2];
	data_local.HEX_array[28] = check_sum_array_bytes[3];

	//data_local.HEX_array[7] = check_sum;

	for (int i = 0; i < 8; ++i)
	{
		check_sum_test += chck_sum_array[i];
	}
	printf("CHECKSUM TEST %lu\n", check_sum_test);
}