#ifndef LPC
#define LPC

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
#include "main.h"
#include "LPC.h"

// LPC handling
uint8_t set_ISP_mode(int print);
uint8_t get_LPC_Info(bool print);
void command_response();
uint8_t set_RUN_mode(int print);

#endif /* LPC */