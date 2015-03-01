#include "main.h"
#include "HM_1X.h"
#include "FTDI_helper.h"

extern FT_DEVICE_LIST_INFO_NODE *devInfo;
extern bool FTDI_open_flag;


extern int command_response_code;

extern FT_STATUS FT_status;
extern DWORD EventDWord;
extern DWORD TxBytes;
extern DWORD BytesWritten;
extern DWORD RxBytes;
extern DWORD BytesReceived;
extern uint8_t RawRxBuffer[2048];
extern uint8_t ParsedRxBuffer[2048];

const int hm_baud_lookup[] = {9600, 19200, 38400, 57600, 115200, 230400};

void HM_1X_main_menu()
{
	// Device properties.
	char name[13];
	int version = 0;
	int baud_rate = 0;
	char parity;
	int stop_bit = 0;
	int mode = 0;
	int role = 0;
	char last_response[128];
	char characteristics[5];
	char mac_address_str[13];

	// Menu variables
	char char_choice[3];
	int int_choice = 0;
	
	// Auto find the current baud of the HM-1X.
	baud_rate = detect_hm1x_baud();

	if (baud_rate)
	{
		get_version_info(&version);
		get_baud_rate(&baud_rate, &stop_bit, &parity);
		get_name(name);
		get_mode(&mode);
		get_characteristics(characteristics);
		get_mac_address(mac_address_str);
	}
	
	do
	{
		system("cls");
		printf("\n");	
		printf("HM-1X -- Main Menu: \n\n");
		printf("1. Get Basic Info\n");
		printf("2. Set Baud\n");
		//printf("3. \n");
		//printf("4. \n");
		printf("5. RSSI\n");
		//printf("6. \n");
		//printf("7. \n");
		//printf("8. \n");
		printf("9. Return\n");

		print_basic_info(
		name,
		&version,
		&baud_rate,
		&parity, 
		&stop_bit, 
		&mode,
		&role, 
		last_response,
		characteristics,
		mac_address_str);

		scanf("%s", char_choice);
		int_choice = atoi(char_choice);

		switch (int_choice)
		{
			case 1:
				get_version_info(&version);
				get_baud_rate(&baud_rate, &stop_bit, &parity);
				get_name(name);
				get_mode(&mode);
				get_characteristics(characteristics);
				get_mac_address(mac_address_str);
				break;
			case 2:
				if(set_hm_baud(&baud_rate) == true){get_baud_rate(&baud_rate, &stop_bit, &parity);}
				break; 
			case 5:
				check_HM_10();
			    break;
			case 6:
			    break;
			case 9:
				main_menu();    
			    break;
			default:printf("Uh, no, sir!");
			    break;
		}
	}while(int_choice !=99);
}

void get_version_info(int * local_version)
{
	// 1. Send the command to get HM-1X firmware version.
	// 2. Get the response.
	// 3. Take just the 3 number version from RX chars.
	// 4. Convert the chars to a single integer.
	// 5. Repeat the command 3 times or until success.
	// 6. Load the version as integer into device property.

	char char_device_info[3];

	int string_start = 8;
	int i = 0;

	char_device_info[0] = '\0';

	// Get version info.
	while(char_device_info[0] == '\0' && i < 3) // Did we ge anything?
	{
		tx_data("AT+VERS?", sizeof("AT+VERS?"), NO_PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		strncpy(char_device_info, RawRxBuffer+string_start, 3);		
		Sleep(100);
		*local_version = atoi(char_device_info);
		printf("HMSoft Version: %i\n", *local_version);
		Sleep(100);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get firmware version. Uh-oh.\n");
		Sleep(1500);
	}
	// Don't leave data in buffer.
	clear_rx_buffer();
}

void get_name(char local_name_string[])
{
	// 1. Send the command to get HM-1X name, which is 12 characters.
	// 2. Get the response.
	// 3. Take the 12 characters and make a string.
	// 4. Repeat the command 3 times or until success.
	// 5. Put the string in a device property.
	
	int string_start = 8;
	int i = 0;

	local_name_string[0] = '\0';

	// Get version info.
	while(local_name_string[0] == '\0' && i < 3) // Did we ge anything?
	{
		tx_data("AT+NAME?", sizeof("AT+NAME?"), NO_PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_name_string, RawRxBuffer+string_start, 12);
		Sleep(100);
		printf("Device Name: %s\n", local_name_string);
		Sleep(100);
		i++;
	}
	// Assure string.
	local_name_string[12] = '\0';
	if (i > 3)
	{
		printf("Failed to get device name. Uh-oh.\n");
		Sleep(1500);
	}
	clear_rx_buffer();
}

void get_baud_rate(int * local_baud_rate, int * local_stop_bit, char * local_parity)
{
	// 1. Send the command to get HM-1X baud rate.
	// 2. Get the response.
	// 3. Take the 1 number version from RX chars.
	// 4. Convert the chars to a single integer.
	// 5. Repeat the command 3 times or until success.
	// 6. Use baud lookup to get baud rate.
	// 7. Load the baud-rate as integer into device property.
	// 8. Use similiar process for stop-bits, and parity.

	char local_baud_rate_str[10];
	char local_stop_bit_str[5];
	char local_parity_str[5];

	int string_start = 7;
	int i = 0;

	local_baud_rate_str[0] = '\0';

	clear_rx_buffer();
	while(local_baud_rate_str[0] == '\0' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+BAUD?", sizeof("AT+BAUD?"), NO_PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_baud_rate_str, RawRxBuffer+string_start, 3);
		*local_baud_rate = atoi(local_baud_rate_str);
		printf("Baud rate: %i\n", *local_baud_rate);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get baud rate. Uh-oh.\n");
		Sleep(1500);
	}
	else{
		switch (*local_baud_rate)
		{
			case 0:
				*local_baud_rate = 9600;
				break;
			case 1:
				*local_baud_rate = 19200;
				break;
			case 2:
				*local_baud_rate = 38400;
				break;
			case 3:
				*local_baud_rate = 57600;
				break;
			case 4:
				*local_baud_rate = 115200;
				break;
			case 8:
				*local_baud_rate = 230400;
				break;
			default:
				*local_baud_rate = 0;
				break;
		}
	}

	clear_rx_buffer();

	string_start = 7;
	i = 0;
	local_stop_bit_str[0] = '\0';

	while(local_stop_bit_str[0] == '\0' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+STOP?", sizeof("AT+STOP?"), NO_PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_stop_bit_str, RawRxBuffer+string_start, 2);
		*local_stop_bit = atoi(local_stop_bit_str);
		printf("Stop bits: %i\n", *local_stop_bit);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get stop bit. Uh-oh.\n");
		Sleep(1500);
	}


	string_start = 7;
	i = 0;
	local_parity_str[0] = '\0';
	int int_pari_bfr = 0;

	while(local_parity_str[0] == '\0' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+PARI?", sizeof("AT+PARI?"), NO_PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_parity_str, RawRxBuffer+string_start, 2);
		int_pari_bfr = atoi(local_parity_str);
		printf("Parity: %i\n", int_pari_bfr);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get parity. Uh-oh.\n");
		Sleep(1500);
	}
	else
	{
		switch(int_pari_bfr)
		{
			case 0:
				*local_parity = 'N';
				break;
			case 1:
				*local_parity = 'E';
				break;
			case 2:
				*local_parity = 'O';
				break;
		}
	}

}

void get_mode(int * local_mode)
{
	// 1. Send the command to get HM-1X mode.
	// 2. Get the response.
	// 3. Take the 1 number version from RX chars.
	// 4. Convert the chars to a single integer.
	// 5. Repeat the command 3 times or until success.
	// 7. Load the mode as integer into mode property.

	char local_mode_str[2];

	int string_start = 7;
	int i = 0;

	local_mode_str[0] = '\0';

	clear_rx_buffer();
	while(local_mode_str[0] == '\0' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+MODE?", sizeof("AT+MODE?"), NO_PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_mode_str, RawRxBuffer+string_start, 1);
		*local_mode = atoi(local_mode_str);
		printf("Mode rate: %i\n", *local_mode);
		i++;
	}
	local_mode_str[1] = '\0';
	if (i > 3)
	{
		printf("Failed to get mode. Uh-oh.\n");
		Sleep(1500);
	}


}

void get_characteristics(char local_characteristics[])
{
	// 1. Send the command to get HM-1X characteristics.
	// 2. Get the response.
	// 3. Take the 4 character version from RX chars.
	// 4. Convert the chars to a string.
	// 5. Repeat the command 3 times or until success.
	// 7. Load the string into characteristics property.
	
	int string_start = 9;
	int i = 0;

	local_characteristics[0] = '\0';

	// Get version info.
	while(local_characteristics[0] == '\0' && i < 3) // Did we ge anything?
	{
		tx_data("AT+CHAR?", sizeof("AT+CHAR?"), NO_PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_characteristics, RawRxBuffer+string_start, 5);
		Sleep(100);
		printf("Device characteristics: %s\n", local_characteristics);
		Sleep(100);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get device name. Uh-oh.\n");
		Sleep(1500);
	}
	clear_rx_buffer(RawRxBuffer, sizeof(RawRxBuffer));
}


void get_role(int * local_role)
{
	// 1. Send the command to get HM-1X role.
	// 2. Get the response.
	// 3. Take the 1 number version from RX chars.
	// 4. Convert the chars to a single integer.
	// 5. Repeat the command 3 times or until success.
	// 7. Load the role as integer into role property.

	char role_str[2];

	int string_start = 7;
	int i = 0;

	role_str[0] = '\0';


	while(role_str[0] == '\0' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+MODE?", sizeof("AT+MODE?"), NO_PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(role_str, RawRxBuffer+string_start, 1);
		*local_role = atoi(role_str);
		printf("Mode: %i\n", *local_role);
		i++;
	}
	role_str[1] = '\0';
	if (i > 3)
	{
		printf("Failed to get mode. Uh-oh.\n");
		Sleep(1500);
	}
	clear_rx_buffer();
}


void get_mac_address(char local_mac_address[])
{
	// 1. Send the command to get HM-1X MAC address.
	// 2. Get the response.
	// 3. Take the 12 character version from RX chars.
	// 4. Convert the chars to a string.
	// 5. Repeat the command 3 times or until success.
	// 7. Load the string into characteristics property.

	int string_start = 8;
	int i = 0;

	local_mac_address[0] = '\0';

	// Get version info.
	while(local_mac_address[0] == '\0' && i < 3) // Did we ge anything?
	{
		tx_data("AT+ADDR?", sizeof("AT+ADDR?"), NO_PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_mac_address, RawRxBuffer+string_start, 12);
		Sleep(100);
		printf("Mac address: %s\n", local_mac_address);
		Sleep(100);
		i++;
	}
	// Assure string.
	local_mac_address[12] = '\0';
	if (i > 3)
	{
		printf("Failed to mac address. Uh-oh.\n");
		Sleep(1500);
	}
	clear_rx_buffer();

}

bool set_hm_baud(int * local_baud_rate)
{
	char baud_set_str[8];
	int ftdi_baud_rate = 0;

	char char_choice[3];
	int int_choice = 0;

	*local_baud_rate = detect_hm1x_baud();

	system("cls");
	printf("\n");	
	printf("Current HM-1X Baud rate: %i\n", *local_baud_rate);
	printf("HM-1X -- Set Baud: \n\n");
	printf("1. 9600\n");
	printf("2. 19200\n");
	printf("3. 38400\n");
	printf("4. 57600\n");
	printf("5. 115200\n");
	printf("6. 230400\n");
	printf("10. Return\n");

	scanf("%s", char_choice);
	int_choice = atoi(char_choice);

	switch (int_choice)
	{
		case 1:
			sprintf(baud_set_str, "AT+BAUD0");
			ftdi_baud_rate = 9600;
			break; 
		case 2:
			sprintf(baud_set_str, "AT+BAUD1");
			ftdi_baud_rate = 19200;
			break;
		case 3:
			sprintf(baud_set_str, "AT+BAUD2");
			ftdi_baud_rate = 38400;
			break; 
		case 4:
			sprintf(baud_set_str, "AT+BAUD3");
			ftdi_baud_rate = 57600;
			break;
		case 5:
			sprintf(baud_set_str, "AT+BAUD4");
			ftdi_baud_rate = 115200;
			break;
		case 6:
			sprintf(baud_set_str, "AT+BAUD8");
			ftdi_baud_rate = 230400;
			break; 		
		case 10:    
		    break;
		default:printf("wrong choice.Enter Again");
		    break;
	}

	// If we leave menu, don't write nuthin'.
	if(int_choice < 7)
	{
		bool changed_baud;
		int count = 0;

		while(!changed_baud && count < 5)
		{
			printf("Attempting to set baud rate.\n");
			tx_data(baud_set_str, 9, NO_PRINT, 0);
			Sleep(100);
			rx(1, NO_PRINT);
			Sleep(120);
			reset_hm1x();
			Sleep(120);
			clear_rx_buffer();
			rx(0, NO_PRINT);
			Sleep(50);
			set_baud_rate_auto(&ftdi_baud_rate);
			Sleep(100);
			changed_baud = ping();
			count++;
		}

		if (changed_baud)
		{	
			*local_baud_rate = ftdi_baud_rate;
			return true;
		}
		else
		{
			printf("Failed to change baud\n");
			Sleep(300);
		}		
	}
	return false;
}



int detect_hm1x_baud()
{
	// 1. Setup local baud rate lookup.
	// 2. 

	char ok_str[3];
	int local_baud_lookup[8];
	int current_hm_baud_rate = 0;

	for (int i = 0; i < 6; ++i)
	{
		local_baud_lookup[i] = hm_baud_lookup[i];
	}

	for (int i = 0; i < 6; ++i)
	{
		set_baud_rate_auto(&local_baud_lookup[i]);
		reset_device(&local_baud_lookup[i]);

		tx_data("AT", sizeof("AT"), NO_PRINT, 0);
		Sleep(130);
		rx(0, 0);
		if (RawRxBuffer[0] == 'O' && RawRxBuffer[1] == 'K')
		{
			printf("%i\n", local_baud_lookup[i]);
			current_hm_baud_rate = local_baud_lookup[i];
			break;
		}
	}
	clear_rx_buffer();
	printf("Local baud rate: %i\n", current_hm_baud_rate);
	Sleep(100);
	return current_hm_baud_rate;
}

void reset_hm1x()
{
	printf("Resetting the HM-1X.\n");
	tx_data("AT+RESET", sizeof("AT+RESET"), NO_PRINT, 0);
	Sleep(180);
	rx(0, 0);
	clear_rx_buffer();
}

bool ping()
{
	clear_rx_buffer();
	// Meant to activate UART flow on devices
	tx_data("AT", sizeof("AT"), NO_PRINT, 1);
	Sleep(150);
	rx(0, 0);
	if (RawRxBuffer[0] == 'O' && RawRxBuffer[1] == 'K')
	{
		clear_rx_buffer();
		return true;
	}
	return false;
}

void check_HM_10()
{
	// Determine if the HM-10 signal is strong enough
	// to attempt an upload.
	char char_RSSI[2];
	int int_RSSI = 0;

	// An HM-10 command to get RSSI.
	tx_chars("AT+RSSI?", sizeof("AT+RSSI?"), NO_PRINT, 0);
	Sleep(500);

	FT_GetStatus(devInfo[connected_device_num].ftHandle, &RxBytes, &TxBytes, &EventDWord);

	if (RxBytes > 0) {
		FT_status = FT_Read(devInfo[connected_device_num].ftHandle,RawRxBuffer,RxBytes,&BytesReceived);
	}
	else
	{
		// Bad RSSI read.
	}		
	
	if(sizeof(RawRxBuffer) < 10)
	{
		char_RSSI[0] = 0;
		char_RSSI[1] = RawRxBuffer[8];
		char_RSSI[2] = RawRxBuffer[8];	
	}
	else if (sizeof(RawRxBuffer) > 9)
	{
		char_RSSI[0] = RawRxBuffer[8];
		char_RSSI[1] = RawRxBuffer[9];
		char_RSSI[2] = RawRxBuffer[10];	
	}
	
	int_RSSI = atoi(char_RSSI);
	//sscanf(char_RSSI, "%D", &int_RSSI);

	if (int_RSSI > 30 && int_RSSI < 60)
	{
 		clearConsole();
		printf("HM-10 signal: ");
		setTextGreen();
		printf("Strong\n");		
	}
	else if (int_RSSI > 60 && int_RSSI < 90)
	{
		clearConsole();
		printf("HM-10 signal: ");
		printf("Medium\n");	
	}
	else if (int_RSSI > 90 && int_RSSI < 120)
	{
		clearConsole();
		printf("HM-10 signal: ");
		setTextRed();
		printf("Weak\n");		
	}
	clearConsole();
}

void print_basic_info(char name[],
	int * version,
	int * baud_rate,
	char * parity, 
	int * stop_bit, 
	int * mode,
	int * role, 
	char last_response[],
	char characteristics[],
	char mac_address_str[])
{
	
		if (*version > 0)
		{
		printf("\n\n");
		printf("Device name: %s\n", name);
		printf("		Firmware Vers:   %i\n", *version);
		printf("		Baud:            %i, %i, %C \n", *baud_rate, *stop_bit, *parity);
		switch(*role)
		{
		case 0:
		printf("		Mode:            Peripheral\n");
			break;
		case 1:
		printf("		Mode:            Master\n");
			break;
		}
		switch(*mode)
		{
		case 0:
		printf("		Mode:            Transmission\n");
			break;
		case 1:
		printf("		Mode:            Collection\n");
			break;
		case 2:
		printf("		Mode:            Remote\n");
			break;	
		}
		printf("		Characteristics: %s\n", characteristics);
		printf("		Mac Address:     ");
		for (int i = 0; i < 12; i)
		{
			for (int j = 0; j < 2; ++j)
			{
				printf("%C", mac_address_str[i]);
				i++;
			}
			if (i < 11)
			{
				printf(":");
			}
		}
		printf("\n");
		}
		else
		{
		printf("	 Auto-detected Baud: %i\n", *baud_rate);	
		}
}

void clear_rx_buffer()
{
	for (int i = 0; i < sizeof(RawRxBuffer); ++i)
	{
		RawRxBuffer[i] = '\0';
	}
}