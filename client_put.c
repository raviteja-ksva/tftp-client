#include <stdio.h>		
#include <string.h>		
#include <stdlib.h> 	
#include <sys/types.h>	
#include <sys/socket.h>	
#include <netdb.h>		
#include <netinet/in.h>	
#include "client_put.h"

void client_put(char *Filename, struct sockaddr_in server, char *Mode, int sock)
{
	int len, server_len, opcode, ssize = 0, n, debug =1, i, j, bcount = 0, tid, datasize = 512, errno;
	unsigned short int count = 0, rcount = 0, acked = 0, ackfreq = 1;
	unsigned char filebuf[MAXDATASIZE + 1];
	unsigned char packetbuf[1][MAXDATASIZE + 12], recvbuf[MAXDATASIZE + 12];
	char filename[128], mode[12], *bufindex;
	struct sockaddr_in ack;

	FILE *fp;			/* pointer to the file we will be sending */

	strcpy (filename, Filename);	//filename stores the name of file
	strcpy (mode, Mode);			// default is "octet" mode

	fp = fopen (filename, "r");
	if (fp == NULL)
	{
		printf ("Client: file: %s not found or permissions denied\n", filename);
		return;
	}

	//get ACK for WRQ packet i.e. Write ReQuest
	for (j = 0; j < RETRIES - 2; j++)
	{
		server_len = sizeof(ack);
		errno = EAGAIN;
		n = -1;
		for (i = 0; errno == EAGAIN && i <= TIMEOUT && n < 0; i++)
		{
			n=recvfrom(sock, recvbuf, sizeof(recvbuf), MSG_DONTWAIT, (struct sockaddr *) &ack, (socklen_t *) &server_len);
			usleep (2000);
		}

		tid = ntohs (ack.sin_port);	//get the tid of the server.
		server.sin_port = htons (tid);	//set the tid for rest of the transfer

		if (n < 0 && errno != EAGAIN)
			printf("Client: could not receive from the server (errno: %d n: %d)\n", errno, n);
		//resend packet
		else if (n < 0 && errno == EAGAIN)
			printf("Client: Timeout waiting for ack (errno: %d n: %d)\n",errno, n);
			//resend packet
		else
		{			/*changed client to server here */
			if (server.sin_addr.s_addr != ack.sin_addr.s_addr)	/* checks to ensure send to ip is same from ACK IP */
			{
				printf("Client: Error recieving ACK (ACK from invalid address)\n");
				j--;		/* in this case someone else connected to our port. Ignore this fact and retry getting the ack */
				continue;
			}
			if (tid != ntohs (server.sin_port))	/* checks to ensure get from the correct TID */
			{
				printf("Client: Error recieving file (data from invalid tid)\n");
				len = err_packet (5, "Unknown transfer ID", buf);
				if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* send the data packet */
					perror ("Client: sendto has returend an error");
				j--;
				continue;		/* we aren't going to let another connection spoil our first connection */
			}

			/* same in the code in the client_get function */
			bufindex = (char *) recvbuf;	//start our pointer going
			if (bufindex++[0] != 0x00)
				printf ("Client: bad first nullbyte!\n");
			opcode = *bufindex++;
			rcount = *bufindex++ << 8;
			rcount &= 0xff00;
			rcount += (*bufindex++ & 0x00ff);
			if (opcode != 4 || rcount != count)	/* ack packet should have code 4 (ack) and should be acking the packet we just sent */
			{
				if(opcode == 5){
						printf("Client: Received an Error Message: %s\n", filebuf);
					}
				printf("Client: Remote host failed to ACK proper data packet # %d (got OP: %d Block: %d)\n",
						 count, opcode, rcount);
				/* sending error message */
				if (opcode > 5)
				{
					len = err_packet (4, "Illegal TFTP operation", buf);
					if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* send the data packet */
					{
						perror ("Client: sendto has returend an error");
					}
				}

			}
			else
			{
				printf("Client: Received ACK- %d\n",rcount);
				break;
			}
		}			//end of else
		printf ("Client: ACK's lost. Resending complete.\n");
	}


	memset (filebuf, 0, sizeof (filebuf));
	while (1)
	{
		acked = 0;
		ssize = fread(filebuf, 1, datasize, fp);
		// if (debug)
		// {
		// 	printf
		// 		("The first data block has been read from the file and will be sent to the server\n");
		// 	printf ("The size read from the file is: %d\n", ssize);
		// }

		count++;			/* count number of datasize byte portions we read from the file */
		if (count == 1)		/* we always look for an ack on the FIRST packet */
			bcount = 0;
		else if (count == 2)	//  The second packet will always start our counter at 0. 
			bcount = 0;
		else
			bcount = (count - 2) % ackfreq;

		// format of data block is - | opcode-03 | Block # | Data |
		sprintf((char *) packetbuf[bcount], "%c%c%c%c", 0x00, 0x03, 0x00, 0x00);
		memcpy((char *) packetbuf[bcount] + 4, filebuf, ssize);
		len = 4 + ssize;
		packetbuf[bcount][2] = (count & 0xFF00) >> 8;	//fill in the count (top number first)
		packetbuf[bcount][3] = (count & 0x00FF);	//fill in the lower part of the count
	
		// printf("Client: Sending packet - %04d \n", count);
		/* send the data packet */
		if (sendto(sock, packetbuf[bcount], len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
		{
			perror ("Client: sendto has returend an error");
			return;
		}
		printf("Client: Packet- %d sent\n", count);

		/* if ((count - 1) == 0 || ((count - 1) % ackfreq) == 0 || ssize != datasize) */
		if (((count) % ackfreq) == 0 || ssize != datasize)
		{

			for (j = 0; j < RETRIES; j++)
			{
				server_len = sizeof (ack);
				errno = EAGAIN;
				n = -1;
				for (i = 0; errno == EAGAIN && i <= TIMEOUT && n < 0; i++)
				{
					n=recvfrom (sock, recvbuf, sizeof (recvbuf), MSG_DONTWAIT,(struct sockaddr *) &ack,(socklen_t *) & server_len);
					usleep (1000);
				}
				if (n < 0 && errno != EAGAIN)
					printf("Client: Server not responding.");
					// need to resend the packet
				else if (n < 0 && errno == EAGAIN)
					printf("Client: ACK not received, Timed out.\n");
					//resend packet
				else
				{		/* sent to wrong destination */
					if (server.sin_addr.s_addr != ack.sin_addr.s_addr)
					{
						printf("Client: Error: ACK received from invalid address)\n");
						j--; 	// resume retries
						continue;
					}
					if (tid != ntohs (server.sin_port))	/* checks to ensure get from the correct TID */
					{
						printf("Client: Error: data received from invalid tid)\n");
						len = err_packet (5, "Unknown transfer ID", buf);
						/* send the data packet */
						if (sendto(sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
							perror ("Client: sendto has returend an error");
						j--;
						continue;	 // optional-  we aren't going to let another connection spoil our first connection */
					}

					/* this formatting code is just like the code in the client_get function */
					bufindex = (char *) recvbuf;	//start our pointer going
					if (bufindex++[0] != 0x00)
						printf ("Client: bad first nullbyte!\n");
					opcode = *bufindex++;

					rcount = *bufindex++ << 8;
					rcount &= 0xff00;
					rcount += (*bufindex++ & 0x00ff);
					// for error handling ...
					if (opcode != 4 || rcount != count)	/* ack packet should have code 4 (ack) and should be acking the packet we just sent */
					{
						printf("Client: incorrect ACK received- %d \n",count);
						/* sending error message */
						if (opcode > 5)
						{
							len = err_packet (4, "Illegal TFTP operation", buf);
							if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* send the data packet */
								perror ("Client: sendto has returend an error");
						}
						/* from here we will loop back and resend */
					}
					else
					{
						printf("Client: Received ACK- %d\n", rcount);
						break;
					}
				}
				for (i = 0; i <= bcount; i++)
				{
					if (sendto(sock, packetbuf[i], len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* resend the data packet */
					{
						perror ("Client: sendto has returend an error");
						return;
					}
					printf("Client: ACK lost. Retransmitting ACK- %d\n",count - bcount + i);
				}
				// printf ("Client: ACK lost. Resending complete.\n");
			}
			/* The ack sending 'for' loop ends here */

		}
		else if (debug)
		{
			printf("Client: Not attempting to recieve ack. Not required. count: %d\n", count);
			n = recvfrom (sock, recvbuf, sizeof (recvbuf), MSG_DONTWAIT, (struct sockaddr *) &ack, (socklen_t *) & server_len);
		}

		if (j == RETRIES)
		{
			printf("Client: ACK Timed out. Suspending transfer\n");
			printf("Client: closing the file...\n");
			fclose (fp);
			return;
		}
		if (ssize != datasize)
			break;
		// for next loop iteration
		memset (filebuf, 0, sizeof (filebuf));
	}
	printf("Client: Closing the file...\n");
	fclose (fp);
	printf ("Client: File sent successfully\n");
	return;
}