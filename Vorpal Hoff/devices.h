#ifndef DEVICES
#define DEVICES

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

// LPC handling
void get_LPC_Info(bool print);
void command_response();
int set_ISP_mode(int print);
int set_RUN_mode(int print);
void wake_devices();
void check_HM_10();

#endif /* DEVICES */