#ifndef HM_1X
#define HM_1X

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
#include <assert.h>
#include "main.h"
#include "FTDI_helper.h"

#define PRINT 1
#define NO_PRINT 0
#define PARSE 1
#define NO_PARSE 0

// HM-1X commands
void HM_1X_main_menu();
int get_version_info(int * local_version);
char get_name(char local_name_string[]);
void get_baud_rate(int * local_baud_rate, int * local_stop_bit, char * local_parity);
int set_hm_baud(int *  local_set_baud);
void wake_devices();
void check_HM_10();

// For getting substring.
char * substr(char * s, int x, int y);

// Clearing the RX buffer
void clear_rx_buffer();

#endif /* HM-1X */