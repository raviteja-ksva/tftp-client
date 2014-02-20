#include <stdio.h>		/*for input and output */
#include <string.h>		/*for string functions */
#include "tftp.h"

/*a function to create the request packet, read or write*/
int req_packet (int opcode, char *filename, char *mode, char buf[]);
/*a function to creat an ACK packet*/
int ack_packet (int block, char buf[]);
/*a function to create the Error packets*/
int err_packet (int err_code, char *err_msg, char buf[]);
/*a function that will print the ip:port pair of the server or client, plus data sent or recieved*/