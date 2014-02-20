#include <stdio.h>	
#include <string.h>	
#include <stdlib.h> 
#include "fill_headers.h"

int req_packet(int opcode, char *filename, char *mode, char header[])
{
	int lenght;
	// format is for RRQ/WRQ : | 01/02 | Filename | 0 | Mode | 0
	lenght = sprintf(header, "%c%c%s%c%s%c", 0x00, opcode, filename, 0x00, mode, 0x00);
	if (lenght == 0)
	{
		printf("Error in creating the request packet\n");	/*could not print to the client buffer */
		exit(-1);
	}
	return lenght;
}

int ack_packet(int block, char buf[])
{
	int packet_len;
	packet_len = sprintf(buf, "%c%c%c%c", 0x00, ACK, 0x00, 0x00);
	buf[2] = (block & 0xFF00) >> 8;
	buf[3] = (block & 0x00FF);
	if (packet_len == 0)
	{
		printf ("Error in creating the ACK packet\n");
		exit(-1);
	}
	printf ("Client: ACK packet created.\n");
	return packet_len;
}


int err_packet(int err_code, char *msg, char buf[])
{
	int packet_len;
	int size = sizeof(buf);
	memset(buf, 0, size);
	packet_len = sprintf(buf, "%c%c%c%c%s%c", 0x00, ERR, 0x00, err_code, msg, 0x00);
	if (packet_len == 0)
	{
		printf ("Error in creating the ACK packet\n");	/*could not print to the client buffer */
		exit (ERROR);
	}
	printf ("Cleint: ERROR packet created.\n");
	return packet_len;
}