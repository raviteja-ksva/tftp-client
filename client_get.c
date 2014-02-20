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
	int len, server_len, opcode, i, j, n, tid = 0, flag = 1, datasize = 512, errno;
	unsigned short int count = 0, rcount = 0, ackfreq = 1;
	unsigned char filebuf[MAXDATASIZE + 1];
	unsigned char packetbuf[MAXDATASIZE + 12];
	char filename[128], mode[12], *bufindex, ackbuf[512];
	struct sockaddr_in data;
	FILE *fp;			/* pointer to the file we will be getting */

	strcpy (filename, p_Filename);
	strcpy (mode, p_Mode);


	// open the file for writing since this is in get file
	fp = fopen (filename, "w");	
	if (fp == NULL)
	{
		printf("Client: Unable to open file- (%s)\n", filename);
		return;
	}
	else
		printf("Client: File: %s created \n", filename);

	bzero(filebuf, sizeof(filebuf));

	n = datasize + 4;
	do
	{
		bzero(packetbuf, sizeof(packetbuf));
		bzero(ackbuf, sizeof(ackbuf));
		// if datasize < full packet => this is last packet to be received 
		if (n != (datasize + 4))	
		{
			printf("Client: last packet identified. \n");
			// Last packet's ACK should have
			// opcode - 04 and block number - 00 
			len = sprintf (ackbuf, "%c%c%c%c", 0x00, 0x04, 0x00, 0x00);

			ackbuf[2] = (count & 0xFF00) >> 8;	//fill in the count (top number first)
			ackbuf[3] = (count & 0x00FF);	//fill in the lower part of the count
			// printf ("sending ACK %04d\n", count);

			if (sendto(sock, ackbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
			{
				perror("Client: sendto has returend an error");
				return;
			}
			printf ("Client: ACK %04d sent\n", count+1);
			goto done;
		}

		count++;

		// loop until we get correct ack OR timed out
		for (j = 0; j < RETRIES; j++)
		{
			server_len = sizeof (data);
			errno = EAGAIN;	// 	i.e., Resource temporarily unavailable
			n = -1;
			//  this for loop will be checking the non-blocking socket until timeout 
			for (i = 0; errno == EAGAIN && i <= TIMEOUT && n < 0; i++)
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

			// means there is an error that is not timed out
			if (n < 0 && errno != EAGAIN)
				printf("The server could not receive from the client\n");

			// means recvfrom timed out
			else if (n < 0 && errno == EAGAIN)
				printf("Client: Receive Timeout (for data)\n");

			else
			{
				if (tid != ntohs (server.sin_port))	/* checks to ensure get from the correct TID */
				{
					printf ("Error recieving file sending error packet\n");
					// send error packet - opcode: 5
					len = sprintf((char *) packetbuf, "%c%c%c%cBad/Unknown TID%c",0x00, 0x05, 0x00, 0x05, 0x00);
					if (sendto (sock, packetbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
						perror("Client: sendto has returend an error");
					j--;
					continue;
				}

				bufindex = (char *) packetbuf;
				if (bufindex++[0] != 0x00)
					printf ("bad first nullbyte!\n");
				opcode = *bufindex++;
				rcount = *bufindex++ << 8;
				rcount &= 0xff00;
				rcount += (*bufindex++ & 0x00ff);

				// copy the rest of the packet (data portion) into our data array
				memcpy ((char *) filebuf, bufindex, n - 4);
				// printf("filebuf content: %s\n", filebuf);	
				printf("Client: received packet - %d\n", rcount);
				if (flag)
				{
					if (n > 516)
						datasize = n - 4;
					else if (n < 516)
						datasize = 512;
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
					printf("Client: invalid data packet (Got OP: %d Block: %d)\n", opcode, rcount);
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
					ackbuf[2] = (count & 0xFF00) >> 8;	//fill in the count (top number first)
					ackbuf[3] = (count & 0x00FF);	//fill in the lower part of the count
					// printf ("ACK %04d sending\n", count);
					if (((count - 1) % ackfreq) == 0)
					{
						if (sendto(sock, ackbuf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
						{
							perror("Client: sendto has returend an error");
							return;
						}
						printf ("Client: ACK %04d sent\n", count);
					}		//check for ackfreq
					else if (count == 1)
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
		if (j == RETRIES)
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
