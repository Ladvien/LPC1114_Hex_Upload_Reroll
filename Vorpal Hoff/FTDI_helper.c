/*------------------------------------------------- FTDI_helper.c ------------
|  Copyright (c) 2015 C. Thomas Brittain
|							aka, Ladvien
*-------------------------------------------------------------------*/

#include "FTDI_helper.h"
#include "main.h"

FT_STATUS ftStatus;
DWORD numDevs;

extern FT_DEVICE_LIST_INFO_NODE *devInfo;

void ftdi_menu()
{
	int baud_rate = 0;	
	char char_choice[3];
	int int_choice = 0;

	bool got_list = false;
	bool connected_flag = false;
	bool close_device_flag = false;
	bool set_baud_flag = false;

	// FTDI Menu
	do
	{
		system("cls");
		printf("FTDI Menu: ");
		if (connected_flag == true)
		{
			printf("       Connected: %lu, N, 1     \n\n", baud_rate);
		}
		else
		{
			printf("       Not Connected:               \n\n");
		}
		printf("1. Quick Connect\n");
		printf("2. Device List\n");
		if (got_list == true) // Only display option if devices list.
		{
		printf("3. Connect Device\n"); 
		}
		if (connected_flag == true) // Only give display if connected.
		{
		printf("4. Close Device\n");
		}
		if (connected_flag == true) // Only give display if connected.
		{
		printf("5. Change baud-rate\n");
		}

		printf("9. Main Menu\n");

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

		// Get user choice.
		scanf("%s", char_choice);

		// Convert string to int for switch statement.
		int_choice = atoi(char_choice);

		switch (int_choice)
		{
			case 1:
				quick_connect();
				baud_rate = 115200;
				connected_flag = true;
			case 2:
				got_list = get_device_list();
				break;
			case 3:
				if (got_list == true) // Only display option if devices listed.
				{			
					connected_flag = connect_device(&baud_rate);
				} 
				break;
			case 4:
				if (connected_flag == true) // Only give display if connected.
				{
					close_device_flag = close_device(&baud_rate);
					if(close_device_flag == true){connected_flag = false;}
					close_device_flag = false;
			    }
			    break;
			case 5:
				if (connected_flag == true) // Only give display if connected.
				{
					set_baud_flag = set_baud_rate(&baud_rate);
					if(close_device_flag == true){connected_flag = false;}
					close_device_flag = false;
			    }
			    break;
			case 9:
				main_menu();    
			    break;
			default:printf("""Bad choice. Hot glue!""");
			    break;
		}
	}while(int_choice !=99);
}

void quick_connect()
{

	int local_baud_rate = 115200;
	// Create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	// get the device information list
	ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
	// Open user's selection.
	// Allocate storage for list based on numDevs.
	devInfo =
	(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
	FT_Open(0, &devInfo[0].ftHandle);
	FT_SetBaudRate(devInfo[0].ftHandle, local_baud_rate);

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
	}

	if (numDevs > 0) {
		
		// allocate storage for list based on numDevs
		devInfo =
		(FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
		
		// get the device information list
		ftStatus = FT_GetDeviceInfoList(devInfo,&numDevs);
		if (ftStatus == FT_OK) {
				printf("Got Devices\n");
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

bool connect_device(int * local_baud_rate)
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
		FT_Open(int_choice, &devInfo[int_choice].ftHandle);

		// Set default baud rate.
		*local_baud_rate = 115200;

		FT_SetBaudRate(devInfo[connected_device_num].ftHandle, *local_baud_rate);

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

bool close_device(int * local_baud_rate)
{
	FT_Close(devInfo[connected_device_num].ftHandle);
	*local_baud_rate = 0;

	if (FT_status != FT_OK)
	{
		printf("Could not close FTDI device.\n");
		Sleep(3000);
		return false;
	}
	else
	{
		return true;
	}
	return false;
}

bool reset_device(int * local_baud_rate)
{
	FT_ResetPort(devInfo[connected_device_num].ftHandle);
	*local_baud_rate = 0;
	
	if (FT_status != FT_OK)
	{
		printf("Could not reset FTDI device.\n");
		Sleep(3000);
		return false;
	}
	else
	{
		// Device reset a success.
		return true;
	}				
	return false; // Just in case.
}

bool set_baud_rate(int * local_baud_rate)
{

	char char_choice[3];
	int int_choice = 0;

	system("cls");
	printf("Set baud: \n");	
	printf("1. 9600\n");
	printf("2. 19200\n");
	printf("3. 38400\n");
	printf("4. 57600\n");
	printf("5. 115200\n");
	printf("6. 230400\n");
	printf("9. Exit\n");

	scanf("%s", char_choice);
	int_choice = atoi(char_choice);

	switch (int_choice)
	{
		case 1:
			*local_baud_rate = 9600;
			break;
		case 2:
			*local_baud_rate = 19200;
			break;
		case 3:
			*local_baud_rate = 38400;
			break;
		case 4:
			*local_baud_rate = 57600;
			break;
		case 5:
			*local_baud_rate = 115200;
			break;
		case 6:
			*local_baud_rate = 230400;
			break;
		case 9:
			return false;
			break;
		default:printf("""Bad choice. Hot glue!""");
		    break;
	}

	FT_SetBaudRate(devInfo[connected_device_num].ftHandle, *local_baud_rate);
	if (FT_OK != FT_OK)
	 {
	 	printf("Unable to change baud-rate\n");
	 	Sleep(3000);
	 	return false;
	 } 
	 else
	 {
	 	return true;
	 }
	 return false;
}