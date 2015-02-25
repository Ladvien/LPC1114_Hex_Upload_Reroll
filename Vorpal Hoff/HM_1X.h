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


struct hm_1x
{
	char name[64];
	int version;
	int baud_rate;
	int stop_bit;
	char last_response[128];
};

// HM-1X commands
void HM_1X_main_menu();
struct hm_1x get_version_info(struct hm_1x local_hm_1x);
struct hm_1x set_hm_baud(struct hm_1x local_hm_1x);
void wake_devices();
void check_HM_10();

// For getting substring.
char * substr(char * s, int x, int y);

#endif /* HM-1X */