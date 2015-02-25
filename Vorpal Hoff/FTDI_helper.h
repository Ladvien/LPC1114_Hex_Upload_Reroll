#ifndef FTDI_HELPER
#define FTDI_HELPER

/*------------------------------------------------- FTDI_helper.h ----------
|  Copyright (c) 2015 C. Thomas Brittain
|							aka, Ladvien
|
|  Module: FTDI_helper 
|
|  Purpose:     
| 			  
|
|  Functions: FUNCTION: IN / OUT
|      
|              
|                      
|                       
|                       
|                       
|                       
|
|  Returns:  
*-------------------------------------------------------------------*/


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h> 
#include <windows.h>
#include <windef.h>
#include <winnt.h>
#include <winbase.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "ftd2xx.h"
#include <sys/time.h>

extern uint8_t ParsedRxBuffer[2048];
extern uint8_t RawRxBuffer[2048];

extern FT_HANDLE handle;
extern FT_STATUS FT_status;
extern DWORD EventDWord;
extern DWORD TxBytes;
extern DWORD BytesWritten;
extern DWORD RxBytes;
extern DWORD BytesReceived;

int connected_device_num;

// Lists FTDI commands.
void ftdi_menu();

void quick_connect();

// Lists FTDI devices connected.
bool get_device_list();
bool connect_device(int * local_baud_rate);
bool close_device(int * local_baud_rate);
bool reset_device(int * local_baud_rate);
bool set_baud_rate(int * local_baud_rate);

#endif /* FTDI_helper.h */