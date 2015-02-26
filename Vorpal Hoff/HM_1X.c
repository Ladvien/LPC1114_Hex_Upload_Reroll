#include "main.h"
#include "HM_1X.h"
#include "FTDI_helper.h"

extern FT_DEVICE_LIST_INFO_NODE *devInfo;

extern int command_response_code;

extern FT_STATUS FT_status;
extern DWORD EventDWord;
extern DWORD TxBytes;
extern DWORD BytesWritten;
extern DWORD RxBytes;
extern DWORD BytesReceived;
extern uint8_t RawRxBuffer[2048];
extern uint8_t ParsedRxBuffer[2048];


void HM_1X_main_menu()
{
	// Device properties.
	char name[13];
	int version = 0;
	int baud_rate = 0;
	int stop_bit = 0;
	char last_response[128];

	char char_choice[3];
	int int_choice = 0;

	do
	{
		system("cls");
		printf("\n");	
		printf("HM-1X -- Main Menu: \n\n");
		printf("1. Get Version Info\n");
		printf("2. Set Baud\n");
		//printf("3. \n");
		//printf("4. \n");
		printf("5. RSSI\n");
		//printf("6. \n");
		//printf("7. \n");
		//printf("8. \n");
		printf("9. Return\n");

		if (version > 0)
		{
		printf("\n\n");
		printf("Device name: %s\n", name);
		printf("		Version:     %i\n", version);
		printf("		Baud:        %i\n", baud_rate);
		}


		scanf("%s", char_choice);
		int_choice = atoi(char_choice);

		switch (int_choice)
		{
			case 1:
				get_version_info(&version);
				get_baud_rate(&baud_rate);
				get_name(name);

				Sleep(200);
				break; 
			case 5:
				check_HM_10();
			    break;
			case 6:
			    break;
			case 9:
				main_menu();    
			    break;
			default:printf("wrong choice.Enter Again");
			    break;
		}
	}while(int_choice !=99);
}

int get_version_info(int * local_version)
{
	// Get device version
	char char_device_info[3];

	int string_start = 8;
	int i = 0;

	char_device_info[0] = '\n';

	// Get version info.
	while(char_device_info[0] == '\n' && i < 3) // Did we ge anything?
	{
		tx_data("AT+VERS?", sizeof("AT+VERS?"), PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		strncpy(char_device_info, RawRxBuffer+string_start, 3);		
		Sleep(100);
		*local_version = atoi(char_device_info);
		printf("\nHMSoft Version: %i\n", *local_version);
		Sleep(100);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get firmware version. Uh-oh.\n");
		Sleep(1500);
	}
	clear_rx_buffer(RawRxBuffer, sizeof(RawRxBuffer));

	return *local_version;
}

char get_name(char local_name_string[])
{
	int string_start = 8;
	int i = 0;

	local_name_string[0] = '\n';

	// Get version info.
	while(local_name_string[0] == '\n' && i < 3) // Did we ge anything?
	{
		tx_data("AT+NAME?", sizeof("AT+NAME?"), PRINT, 0);
		Sleep(100);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_name_string, RawRxBuffer+string_start, 12);
		Sleep(100);
		printf("\nDevice Name: %s\n", local_name_string);
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

int get_baud_rate(int * local_baud_rate)
{

	// Get baud rate. 
	char local_baud_rate_str[10];
	int string_start = 7;
	int i = 0;

	local_baud_rate_str[0] = '\n';

	clear_rx_buffer();
	while(local_baud_rate_str[0] == '\n' && i < 3) // Did we ge anything?
	{
		Sleep(100);
		tx_data("AT+BAUD?", sizeof("AT+BAUD?"), PRINT, 0);
		Sleep(150);
		rx(NO_PARSE, NO_PRINT);
		// Max name is == 12 chars.
		strncpy(local_baud_rate_str, RawRxBuffer+string_start, 3);
		Sleep(100);
		*local_baud_rate = atoi(local_baud_rate_str);
		printf("\nBaud rate: %i\n", *local_baud_rate);
		Sleep(400);
		i++;
	}
	if (i > 3)
	{
		printf("Failed to get baud rate. Uh-oh.\n");
		Sleep(1500);
	}

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
		case 5:
			*local_baud_rate = 4800;
			break;
		case 6:
			*local_baud_rate = 2400;
			break;
		case 7:
			*local_baud_rate = 1200;
			break;
		case 8:
			*local_baud_rate = 230400;
			break;
		default:
			*local_baud_rate = 0;
			break;
	}

	clear_rx_buffer();
	return *local_baud_rate;
}

int set_hm_baud(int * local_set_baud)
{

	char char_choice[3];
	int int_choice = 0;

	do
	{
		system("cls");
		printf("\n");	
		printf("HM-1X -- Main Menu: \n\n");
		printf("1. Get Version Info\n");
		printf("2. Set Baud\n");
		//printf("3. \n");
		//printf("4. \n");
		printf("5. RSSI\n");
		//printf("6. \n");
		//printf("7. \n");
		//printf("8. \n");
		printf("9. Return\n");

		scanf("%s", char_choice);
		int_choice = atoi(char_choice);

		switch (int_choice)
		{
			case 1:
				//hm_1x = get_version_info(hm_1x);
				break; 
			case 5:
				//check_HM_10();
			    break;
			case 6:
			    break;
			case 9:
				//main_menu();    
			    break;
			default:printf("wrong choice.Enter Again");
			    break;
		}
	}while(int_choice !=99);

	// Get device version
	char char_device_info[5];
	int int_RSSI = 0;

	// Get version info.
	tx_data("AT", sizeof("AT"), PRINT, 0);
	Sleep(300);

	rx(1, 1);
}

void wake_devices()
{
	// Meant to activate UART flow on devices
	tx_chars("Wake", sizeof("Wake"), NO_PRINT, 10);
	Sleep(500);
	rx(NO_PARSE, PRINT);
	printf("Waking Devices...\n");
}

void check_HM_10()
{
	// Determine if the HM-10 signal is strong enough
	// to attempt an upload.
	char char_RSSI[2];
	int int_RSSI = 0;

	// An HM-10 command to get RSSI.
	tx_chars("AT+RSSI?", sizeof("AT+RSSI?"), PRINT, 0);
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

char * substr(char * s, int x, int y)
{
    char * ret = malloc(strlen(s) + 1);
    char * p = ret;
    char * q = &s[x];

    assert(ret != NULL);

    while(x  < y)
    {
        *p++ = *q++;
        x ++; 
    }

    *p++ = '\0';

    return ret;
}

void clear_rx_buffer()
{
	for (int i = 0; i < sizeof(RawRxBuffer); ++i)
	{
		RawRxBuffer[i] = '\n';
	}
}