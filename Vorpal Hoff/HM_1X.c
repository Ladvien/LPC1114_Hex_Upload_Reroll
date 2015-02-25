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
	struct hm_1x hm_1x;
	char char_choice[3];
	int int_choice = 0;
	hm_1x.version = 0;

	do
	{
		system("cls");
		printf("\n");	
		printf("HM-1X -- Main Menu: ");
		if (hm_1x.version > 0)
		{
		printf("                Version: %i\n\n", hm_1x.version);
		}
		else
		{printf("\n\n");}
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
				hm_1x = get_version_info(hm_1x);
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

struct hm_1x get_version_info(struct hm_1x local_hm_1x)
{
	// Get device version
	char char_device_info[3];
	int int_RSSI = 0;

	// Get version info.
	tx_data("AT+VERS?", sizeof("AT+VERS?"), PRINT, 0);
	Sleep(100);
	rx(0,0);
	char * version = substr(RawRxBuffer,8,11);
	strcpy(char_device_info, version);
	Sleep(100);
	local_hm_1x.version = atoi(char_device_info);
	printf("\nHMSoft Version: %i\n", local_hm_1x.version);
	Sleep(5000);

	return local_hm_1x;
}

struct hm_1x set_hm_baud(struct hm_1x local_hm_1x)
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