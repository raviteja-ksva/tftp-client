/*
	This function implements the get functionality 
	of the client. i.e., gets required file from
	server and creates it at client end.
	This function is called from client.c and 
	inturn uses fill_headers.c
*/

#include <stdio.h>		
#include <string.h>		
#include <stdlib.h> 	
#include <sys/types.h>	
#include <sys/socket.h>	
#include <netdb.h>		
#include <netinet/in.h>	
#include "client_get.h"

void client_get(char *p_Filename, struct sockaddr_in server, char *p_Mode, int sock)
{
	/* local variables */
	int len, server_len, opcode, i, j, n, tid = 0, flag = 1, size_data = 512;
	unsigned short int ACK_no = 0, block_no = 0;
	unsigned char filebuf[1024 + 1];
	unsigned char packetbuf[1024 + 12];
	char filename[128], mode[12], *bufindex, ackbuf[512];
	struct sockaddr_in data;
	FILE *fp;			/* pointer to the file we will be getting */

	strcpy(filename, p_Filename);
	strcpy(mode, p_Mode);

	// open the file for writing since this is in get file
	fp = fopen (filename, "w");	
	if(fp == NULL)
	{
		printf("Client: Unable to open file- (%s)\n", filename);
		return;
	}
	else
		printf("Client: File: %s created \n", filename);

	bzero(filebuf, sizeof(filebuf));

	n = size_data + 4;	// 4 bytes header
	do
	{
		bzero(packetbuf, sizeof(packetbuf));
		bzero(ackbuf, sizeof(ackbuf));
		// if size_data < full packet => this is last packet to be received 
		if (n != (size_data + 4))	
		{
			printf("Client: last packet identified. \n");
			// Last packet's ACK should have
			// opcode - 04 and block number - 00 
			len = sprintf (ackbuf, "%c%c%c%c", 0x00, 0x04, 0x00, 0x00);

			ackbuf[2] = (ACK_no & 0xFF00) >> 8;	//fill in the first 8 bits of ACK_no
			ackbuf[3] = (ACK_no & 0x00FF);	//fill in the last part of the ACK_no
			// printf ("sending ACK %04d\n", ACK_no);

			if (sendto(sock, ackbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
			{
				perror("Client: sendto has returend an error");
				return;
			}
			printf ("Client: ACK %04d sent\n", ACK_no+1);
			goto done;
		}

		ACK_no++;

		// loop until we get correct ack OR timed out
		for (j = 0; j < RETRY; j++)
		{
			server_len = sizeof (data);
			n = -1;
			//  this for loop will be checking the non-blocking socket until timeout 
			for (i = 0; i <= TIMEOUT && n < 0; i++)
			{
				n =	recvfrom (sock, packetbuf, sizeof (packetbuf) - 1,
							MSG_DONTWAIT, (struct sockaddr *) &data, (socklen_t *) & server_len);
				usleep (1000);
			}
			if (!tid)
			{
				tid = ntohs(data.sin_port);	//get the tid of the server.
				server.sin_port = htons (tid);	//set the tid for rest of the transfer 
			}

			// means recvfrom timed out
			if (n < 0)
				printf("Client: Receive Timeout (for data)\n");

			else
			{

				bufindex = (char *)packetbuf;
				bufindex++; // first byte = 0
				opcode = *bufindex++;
				block_no = *bufindex++ << 8;
				block_no &= 0xff00;
				block_no += (*bufindex++ & 0x00ff);

				// copy the rest of the packet (data portion) into our data array
				memcpy ((char *) filebuf, bufindex, n - 4);
				// printf("filebuf content: %s\n", filebuf);	
				printf("Client: received packet - %d\n", block_no);
				if (flag)
				{
					if (n > 516)
						size_data = n - 4;
					else if (n < 516)
						size_data = 512;
					flag = 0;
				}
				if (opcode != 3)	/* ack packet should have code 3 (data) and should be ack+1 the packet we just sent */
				{
					if(opcode == 5){
						printf("Client: Received an Error Message: %s\n", filebuf);
						printf("Client: Closing file- %s\n", filename);
						fclose (fp);
						return;
					}
					printf("Client: invalid data packet (Got OP: %d Block: %d)\n", opcode, block_no);
					/* send error message */
					if (opcode > 5)
					{
						len =sprintf((char *) packetbuf,"%c%c%c%cIllegal operation%c",0x00, 0x05, 0x00, 0x04, 0x00);
						if (sendto (sock, packetbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
							perror("Client: sendto has returend an error");
					}
				}
				else
				{
					// ACK opcode: 4 , expected block #
					len = sprintf (ackbuf, "%c%c%c%c", 0x00, 0x04, 0x00, 0x00);
					ackbuf[2] = (ACK_no & 0xFF00) >> 8;	//fill in the ACK_no (top number first)
					ackbuf[3] = (ACK_no & 0x00FF);	//fill in the lower part of the ACK_no
					// printf ("ACK %04d sending\n", ACK_no);
					if (sendto(sock, ackbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
					{
						perror("Client: sendto has returend an error");
						return;
					}
					printf ("Client: ACK %04d sent\n", ACK_no);
					if (ACK_no == 1)
					{
						if (sendto(sock, ackbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
						{
							perror("Client: sendto has returend an error");
							return;
						}
						printf ("Client: ACK 1 sent\n");
					}
					break;
				}		//end of else
			}
		}
		if (j == RETRY)
		{
			printf ("Receive time Out\n");
			fclose (fp);
			return;
		}
	}
	// if it doesn't write the file the length of the packet received less 4 then it didn't work 
	while (fwrite (filebuf, 1, n - 4, fp) == n - 4);
	fclose (fp);
	sync ();
	printf("Client: file failed to recieve properly\n");
	return;

done:

	fclose (fp);
	sync ();
	printf ("Client: File received successfully\n");
	return;
}
