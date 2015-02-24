#include "FTDI_helper.h"
#include "main.h"
/*------------------------------------------------- FTDI_helper.c ------------
|  Copyright (c) 2015 C. Thomas Brittain
|							aka, Ladvien
*-------------------------------------------------------------------*/
FT_STATUS ftStatus;
DWORD numDevs;

extern FT_DEVICE_LIST_INFO_NODE *devInfo;

void ftdi_menu()
{


	char char_choice[3];
	int int_choice = 0;

	bool got_list = false;
	bool connected_flag = false;

	int connected_device_num = 0;

	do
	{
		system("cls");
		printf("FTDI Menu: \n\n");
		printf("1. Device List\n");
		if (got_list == true)
		{
		printf("2. Connect Device\n");
		}
		
		printf("9. Return\n");

		if (got_list == true && connect_device == false)
		{
			printf("\n");
			printf("\n\nConnected FTDI:");
			for (int i = 0; i < numDevs; i++) {
				printf("\nDevice: %d:\n",i);
				printf(" 	Flags:         0x%02X\n",devInfo[i].Flags);
				printf(" 	Type:          0x%02X\n",devInfo[i].Type);
				printf(" 	ID:            0x%02X\n",devInfo[i].ID);
				printf(" 	Local ID:      0x%02X\n",devInfo[i].LocId);
				printf(" 	Serial Number: %s\n",devInfo[i].SerialNumber);
				printf(" 	Description:   %s\n",devInfo[i].Description);
				printf(" 	ftHandle =     0x%02X\n",devInfo[i].ftHandle);
			}
		}
		if (connected_flag == true)
		{
			printf("\n");
			printf("Connected Device: %d:\n", connected_device_num);
			printf(" 	Flags:         0x%02X\n",devInfo[connected_device_num].Flags);
			printf(" 	Type:          0x%02X\n",devInfo[connected_device_num].Type);
			printf(" 	ID:            0x%02X\n",devInfo[connected_device_num].ID);
			printf(" 	Local ID:      0x%02X\n",devInfo[connected_device_num].LocId);
			printf(" 	Serial Number: %s\n",devInfo[connected_device_num].SerialNumber);
			printf(" 	Description:   %s\n",devInfo[connected_device_num].Description);
			printf(" 	ftHandle =     0x%02X\n",devInfo[connected_device_num].ftHandle);

		}

		scanf("%s", char_choice);
		int_choice = atoi(char_choice);

		printf("%i\n", int_choice);
		switch (int_choice)
		{
			case 1:
				got_list = get_device_list();
				break; 
			case 2:
				connected_flag = connect_device(connected_device_num);
			    break;
			case 6:
			    break;
			case 9:
				main_menu();    
			    break;
			default:printf("""Bad choice. Hot glue!""");
			    break;
		}
	}while(int_choice !=99);
}

	
	
bool get_device_list()
{
	// Create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);

	if (ftStatus == FT_OK) {
		printf("Number of devices is %d\n",numDevs);
	}
	else {
		printf("Failed to get FTDI device list.\n");
	}

	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus == FT_OK) {
		printf("Number of devices is %d\n",numDevs);
		Sleep(50);
	}

	if (numDevs > 0) {
		
		// allocate storage for list based on numDevs
		devInfo =
		(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
		
		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
		if (ftStatus == FT_OK) {
				printf("Got Devices\n");
				Sleep(50);
			}
		else
			{
				printf("Failed to get device list.\n");
				Sleep(3000);
			}
		}
	if (numDevs > 0)
	{
		// Set flag if we got at least on device.
		return true;	
	}
	return false;
}

bool connect_device(int connected_device_num)
{

	char char_choice[3];
	int int_choice = 0;

	bool connected_flag = false;

	system("cls");
	printf("Which device # (0-8)?\n\n");
	printf("9. Return\n");

	printf("\n\nConnected FTDI:");
	for (int i = 0; i < numDevs && i < 8; i++) {
		printf("\nDevice: %d:\n",i);
		printf(" 	Flags:         0x%02X\n",devInfo[i].Flags);
		printf(" 	Type:          0x%02X\n",devInfo[i].Type);
		printf(" 	ID:            0x%02X\n",devInfo[i].ID);
		printf(" 	Local ID:      0x%02X\n",devInfo[i].LocId);
		printf(" 	Serial Number: %s\n",devInfo[i].SerialNumber);
		printf(" 	Description:   %s\n",devInfo[i].Description);
		printf(" 	ftHandle =     0x%02X\n",devInfo[i].ftHandle);
	}

	scanf("%s", char_choice);
	int_choice = atoi(char_choice);

	// Limit list to 9 devices.  Really, who has more connected at once?
	if (int_choice == 9)
	{
		return false;
	}
	else if (int_choice > -1 && int_choice < 9 && int_choice <= numDevs)
	{
		// Open user's selection.
		FT_Open(int_choice, &handle);
		if (FT_status != FT_OK)
		{
			printf("Could not open FTDI device #%i.\n", int_choice);
			Sleep(3000);
		}
		else
		{
			connected_device_num = int_choice;
			return true;
		}
	}
	else
	{
		return false;
	}
	return false;
}