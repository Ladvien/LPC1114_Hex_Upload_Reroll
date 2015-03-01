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

// HM-1X GET commands
void HM_1X_main_menu();
void get_version_info(int * local_version);
void get_name(char local_name_string[]);
void get_baud_rate(int * local_baud_rate, int * local_stop_bit, char * local_parity);
void get_mode(int * local_mode);
void get_role(int * role);
void get_characteristics(char local_characteristics[]);
void get_mac_address(char local_mac_address[]);
int detect_hm1x_baud();


// HM-1X SET commands
bool set_hm_baud(int * local_baud_rate);
bool ping();
void check_HM_10();
void reset_hm1x();


// Print info.
void print_basic_info(char name[],
	int * version,
	int * baud_rate,
	char * parity, 
	int * stop_bit, 
	int * role,
	int * mode, 
	char last_response[],
	char characteristics[],
	char mac_address_str[]);

// Clearing the RX buffer
void clear_rx_buffer();


#endif /* HM-1X */