/*
	These functions only creates respective 
	packets given the required arguments.
	These functions are used in every other function
*/

#include <stdio.h>	
#include <string.h>	
#include <stdlib.h> 
#include "fill_headers.h"

int request_packet(int opcode, char *filename, char *mode, char header[])
{
	int len;
	// format is for RRQ/WRQ : | 01/02 | Filename | 0 | Mode | 0
	len = sprintf(header, "%c%c%s%c%s%c", 0x00, opcode, filename, 0x00, mode, 0x00);
	return len;
}

int error_pkt(int err_code, char *msg, char buf[])
{
	int len;
	int size = sizeof(buf);
	memset(buf, 0, size);
	len = sprintf(buf, "%c%c%c%c%s%c", 0x00, ERR, 0x00, err_code, msg, 0x00);
	// printf ("Cleint: ERROR packet created.\n");
	return len;
}