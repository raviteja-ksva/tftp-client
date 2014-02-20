/*
	This function implements the PUT functionality
	i.e., puts required file to server.
	Reads a block of data from required file, then
	makes data packets and sends them to server.
	Most errors are also handled here.
*/

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
	int len, server_len, opcode, ssize = 0, n, i,j,transfer_id, size_data = 512;
	unsigned short int block_no = 0, ACK_no = 0;
	unsigned char filebuf[1024 + 1];
	unsigned char ACK_packet[1024 + 12], packet[1024 + 12];
	char filename[128], mode[12], *bufptr;
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
	for (j = 0; j < RETRY - 2; j++)
	{
		server_len = sizeof(ack);
		n = -1;
		for (i = 0; i <= TIMEOUT && n < 0; i++)
		{
			n=recvfrom(sock, packet, sizeof(packet), MSG_DONTWAIT, (struct sockaddr *) &ack, (socklen_t *) &server_len);
			usleep (2000);
		}

		transfer_id = ntohs (ack.sin_port);	//get the transfer_id of the server.
		server.sin_port = htons (transfer_id);	//set the transfer_id for rest of the transfer

		if(n < 0)
			printf("Client: Timed out ACK\n");
			//resend packet
		else
		{			/*changed client to server here */
			if (server.sin_addr.s_addr != ack.sin_addr.s_addr)	/* checks to ensure send to ip is same from ACK IP */
			{
				printf("Client: Error recieving ACK (ACK from invalid address)\n");
				j--;		/* in this case someone else connected to our port. Ignore this fact and retry getting the ack */
				continue;
			}
			if (transfer_id != ntohs (server.sin_port))	/* checks to ensure get from the correct transfer_id */
			{
				printf("Client: Error recieving file (data from invalid transfer_id)\n");
				len = error_pkt (5, "Unknown transfer ID", buf);
				if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* send the data packet */
					perror ("Client: sendto has returend an error");
				j--;
				continue;		/* we aren't going to let another connection spoil our first connection */
			}

			/* same in the code in the client_get function */
			bufptr = (char *) packet;	//start our pointer going
			bufptr++;
			opcode = *bufptr++;
			ACK_no = *bufptr++ << 8;
			ACK_no &= 0xff00;
			ACK_no += (*bufptr++ & 0x00ff);
			if (opcode != 4 || ACK_no != block_no)	/* ack packet should have code 4 (ack) and should be acking the packet we just sent */
			{
				if(opcode == 5)
					printf("Client: Received an Error Message: %s\n", filebuf);
				printf("Client: Host failed to ACK correct data packet\n");
			}
			else
			{
				printf("Client: Received ACK- %d\n",ACK_no);
				break;
			}
		}			//end of else
		printf ("Client: ACK's lost. Resending complete.\n");
	}

	memset (filebuf, 0, sizeof (filebuf));		// make buffer zeros
	while (1)
	{
		ssize = fread(filebuf, 1, size_data, fp);
		block_no++;			/* count number of size_data byte portions we read from the file */

		// format of data block is - | opcode-03 | Block # | Data |
		sprintf((char *) ACK_packet, "%c%c%c%c", 0x00, 0x03, 0x00, 0x00);
		memcpy((char *) ACK_packet + 4, filebuf, ssize);
		len = 4 + ssize;
		ACK_packet[2] = (block_no & 0xFF00) >> 8;
		ACK_packet[3] = (block_no & 0x00FF);	
	
		// printf("Client: Sending packet - %04d \n", count);
		/* send the data packet */
		if (sendto(sock, ACK_packet, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
		{
			perror ("Client: sendto has returend an error");
			return;
		}
		printf("Client: Packet- %d sent\n", block_no);
		int j;
		for (j = 0; j < RETRY; j++)
		{
			server_len = sizeof (ack);
			n = -1;
			for (i = 0; i <= TIMEOUT && n < 0; i++)
			{
				n=recvfrom(sock, packet, sizeof(packet), MSG_DONTWAIT,(struct sockaddr *) &ack,(socklen_t *) & server_len);
				usleep (1000);
			}
			if (n < 0 )
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
				if (transfer_id != ntohs (server.sin_port))	/* checks to ensure get from the correct transfer_id */
				{
					printf("Client: Error: data received from invalid transfer_id)\n");
					len = error_pkt(5, "Unknown transfer ID", buf);
					/* send the data packet */
					if (sendto(sock, buf, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)
						perror ("Client: sendto has returend an error");
					j--;
					continue;	 // optional-  we aren't going to let another connection spoil our first connection */
				}

				/* this formatting code is just like the code in the client_get function */
				bufptr = (char *) packet;	//start our pointer going
				if (bufptr++[0] != 0x00)
					printf ("Client: bad first nullbyte!\n");
				opcode = *bufptr++;

				ACK_no = *bufptr++ << 8;
				ACK_no &= 0xff00;
				ACK_no += (*bufptr++ & 0x00ff);
				// for error handling ...
				if (opcode != 4 || ACK_no != block_no)	// ack packet should have code 4 
					printf("Client: incorrect ACK received- %d \n",block_no);
				else
				{
					printf("Client: Received ACK- %d\n", ACK_no);
					break;
				}
			}
			if (sendto(sock, ACK_packet, len, 0, (struct sockaddr *) &server, sizeof (server)) != len)	/* resend the data packet */
			{
				perror ("Client: sendto has returend an error");
				return;
			}
			printf("Client: ACK lost. Retransmitting ACK- %d\n",block_no + i);
			// printf ("Client: ACK lost. Resending complete.\n");
		}
		/* The ack sending 'for' loop ends here */
		if (j == RETRY)
		{
			printf("Client: ACK Timed out. Suspending transfer\n");
			printf("Client: closing the file...\n");
			fclose (fp);
			return;
		}
		if (ssize != size_data)
			break;
		// for next loop iteration
		memset (filebuf, 0, sizeof (filebuf));
	}
	printf("Client: Closing the file...\n");
	fclose (fp);
	printf ("Client: File sent successfully\n");
	return;
}