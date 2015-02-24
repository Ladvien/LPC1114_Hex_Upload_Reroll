#include "devices.h"
extern FT_DEVICE_LIST_INFO_NODE *devInfo;
extern uint8_t ParsedRxBuffer[2048];
extern uint8_t RawRxBuffer[2048];
extern int command_response_code;

extern FT_STATUS FT_status;
extern DWORD EventDWord;
extern DWORD TxBytes;
extern DWORD BytesWritten;
extern DWORD RxBytes;
extern DWORD BytesReceived;
extern uint8_t RawRxBuffer[2048];
extern uint8_t ParsedRxBuffer[2048];


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

int set_ISP_mode(int print)
{
	// Remotely set LPC into ISP mode.

	// Test completeness of command.
	int successful = 0;

	printf("Starting LPC ISP.");

	// Let's attempt 3x to successfully set ISP mode
	for (int i = 0; i < 5; ++i)
	{	
		// "AT+PIO30"
		tx_chars(HM_ISP_LOW, sizeof(HM_ISP_LOW), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");

		// "AT+PIO20"
		tx_chars(HM_LPC_RESET_LOW, sizeof(HM_LPC_RESET_LOW), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");
		
		// "AT+PIO21"
		tx_chars(HM_LPC_RESET_HIGH, sizeof(HM_LPC_RESET_HIGH), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");
		
		// Synchronized check.
		tx_chars(LPC_CHECK, sizeof(LPC_CHECK), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");

		// Tell the LPC we are synchronized.
		tx_chars(Synchronized, sizeof(Synchronized), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");

		// Set crystal
		tx_chars("12000\n", sizeof("12000\n"), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");

		// Let's turn off ECHO.
		tx_chars("A 0\n", sizeof("A 0\n"), print, 10);
		Sleep(100);
		rx(PARSE, print);		
		printf(".");

		// Set baud
		//tx_chars("9600\n", sizeof("9600\n"), 0);
		//Sleep(500);
		//setTextGreen();
		//printf("%s\n", ParsedRxBuffer);

		Sleep(100);

		// If all the HM-10 commands responded then
		// the ISP mode should successfully be set.
		if (successful > 5)
		{
			OK();
			return 1;	
		}
		// Let's try 3x.
		else
		{
			Failed();
			successful = 0;
			printf("Retrying.");
			clearBuffers();
			Sleep(500);
		}
	}
	return 0;
}

int set_RUN_mode(int print)
{
	// Remotely set LPC into ISP mode.

	// Test completeness of command.
	int successful = 0;

	printf("Setting RUN mode.");

	// Let's attempt 3x to successfully set ISP mode
	for (int i = 0; i < 5; ++i)
	{	
		// "AT+PIO31"
		tx_chars(HM_ISP_HIGH, sizeof(HM_ISP_HIGH), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");

		// "AT+PIO20"
		tx_chars(HM_LPC_RESET_LOW, sizeof(HM_LPC_RESET_LOW), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");
		
		// "AT+PIO21"
		tx_chars(HM_LPC_RESET_HIGH, sizeof(HM_LPC_RESET_HIGH), print, 0);
		Sleep(100);
		successful += rx(PARSE, print);
		printf(".");


		if (successful > 3)
		{
			OK();
			return 1;	
		}
		// Let's try 3x.
		else
		{
			Failed();
			successful = 0;
			printf("Retrying.");
			clearBuffers();
			Sleep(500);
		}
	}
	return 0;
}

void get_LPC_Info(bool print)
{
	int successful = 0;
	uint8_t PartID[256];
	uint8_t UID[256];
	uint8_t BootVersion[256];

	for (int i = 0; i < 3; ++i)
	{	
		// Read Part ID
		tx_chars("N\n", sizeof("N\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(PartID, ParsedRxBuffer);
		
		// Read UID
		tx_chars("J\n", sizeof("J\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(UID, ParsedRxBuffer);

		// Boot Version
		tx_chars("K\n", sizeof("K\n"), print, 0);
		Sleep(500);
		successful += rx(PARSE, print);
		copy_string(BootVersion, ParsedRxBuffer);
		

		clearConsole();

		if (successful > 2)
		{
			printf("Device info successfully read: \n\n");
			printf("Boot Version: %s\n", BootVersion);
			printf("UID:          %s\n", UID);
			printf("Part Number:  %s\n", PartID);
			break;	
		}
		else
		{
			successful = 0;
			printf("Attempt %i to get LPC info and failed. Retrying.\n", i);
			clearBuffers();
			Sleep(500);
		}
	}


}

void command_response()
{
	// Clear ParsedRxBuffer.
	for (int i = 0; i != sizeof(ParsedRxBuffer); ++i)
	{
		ParsedRxBuffer[i] = 0;
	}
		// Clear ParsedRxBuffer.
	for (int i = 0; i != sizeof(RawRxBuffer); ++i)
	{
		RawRxBuffer[i] = 0;
	}

	printf("\n");
	// Pulls from global command_reponse_code, which is
	// set in parseRx().
	switch(command_response_code)
	{
		// 1
		case RESP_CMD_SUCCESS:
			printf("Command Success");
			break;
		// 2
		case RESP_INVALID_COMMAND:
			printf("Invalid Command");
			break;
		// 3
		case RESP_SRC_ADDR_ERROR:
			printf("Source Address Error");
			break;
		// 4
		case RESP_SRC_ADDR_NOT_MAPPED:
			printf("Source Address Not Mapped");
			break;
		// 5
		case RESP_DST_ADDR_NOT_MAPPED:
			printf("Destination Address Not Mapped");
			break;
		// 6
		case RESP_COUNT_ERROR:
			printf("Count Error");
			break;
		// 7
		case RESP_INVALID_SECTOR:
			printf("Invalid Sector");
			break;
		// 8
		case RESP_SECTOR_NOT_BLANK:
			printf("Sector Not Blank");
			break;
		// 9
		case RESP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION:
			printf("Sector Not Prepared for Write Operation");
			break;
		// 10
		case RESP_COMPARE_ERROR:
			printf("Compare Error");
			break;
		// 11
		case RESP_BUSY:
			printf("Busy");
			break;
		// 12
		case RESP_PARAM_ERROR:
			printf("Parameter Error");
			break;
		// 13
		case RESP_ADDR_ERROR:
			printf("Address Error");
			break;
		// 14
		case RESP_ADDR_NOT_MAPPED:
			printf("Address Not Mapped");
			break;
		// 15
		case RESP_CMD_LOCKED:
			printf("Command Locked");
			break;
		// 16
		case RESP_INVALID_CODE:
			printf("Invalid Code");
			break;
		// 17
		case RESP_INVALID_BAUD_RATE:
			printf("Invalid Baud Rate");
			break;
		// 18
		case RESP_INVALID_STOP_BIT:
			printf("Invalid Stop Bit");
			break;
		// 19
		case RESP_CODE_READ_PROTECTION_ENABLED:
			printf("Code Read Protection Enabled");
			break;			
	}
	// Clear for next use.
	command_response_code = 0;
}