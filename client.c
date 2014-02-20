/*
	This file takes in command line arguments,
	parses available options and sets destination address,
	mode of operation etc, opens/creates required file
	in appropriate mode
*/
	
#include <stdio.h>		
#include <string.h>		
#include <stdlib.h> 	
#include <sys/types.h>	
#include <sys/socket.h>	
#include <netdb.h>		
#include <netinet/in.h>
#include "fill_headers.h"
#include "client_get.h"
#include "client_put.h"
#include "opcodes.h"

int port = 69;

int main (int argc, char **argv)
{
	int sock, len2, len;
	char opcode, filename[196], mode[12] = "octet"; // default is octet
	struct hostent *host;
	struct sockaddr_in server;
	char* host_addr = "127.0.0.1";
	FILE *fp;

	if (argc < 2)
	{
		printf("invalid format of command\n");
		printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
		return 0;
	}

	argv++;
	argc--;

	if (strcmp(argv[0], "-h") == 0)
	{
		if (argc==1) 
		{
			printf("invalid format of command\n");
			printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
			return 0;
		}
		host_addr = argv[1];
		argv+=2;
		argc-=2;
	}
	if (strcmp(argv[0], "-P")==0){
		if (argc==1) 
		{
			printf("invalid format of command\n");
			printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
			return 0;
		}
		port = atoi(argv[1]);

		argv+=2;
		argc-=2;
	}
	if (strcmp(argv[0], "-g")==0){
		if (argc==1) 
		{
			printf("invalid format of command\n");
			printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
			return 0;
		}
		strncpy (filename, argv[1], sizeof (filename) - 1);
		opcode = RRQ;
		argv+=2;
		argc-=2;
	} else if (strcmp(argv[0], "-p")==0){
		if (argc==1) 
		{
			printf("invalid format of command\n");
			printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
			return 0;
		}
		strncpy (filename, argv[1], sizeof (filename) - 1);
		opcode = WRQ;
		argv+=2;
		argc-=2;
	} else if (strcmp(argv[0], "-n")==0){
		strncpy (mode, "netascii", sizeof (mode) - 1);
		printf ("Client: The mode is set to netascii\n");
	} else{
		printf("Command format: ./client -h [host_addr] -P [port] -g|-p [filename] [-n] \n");
		return (0);
	}

	printf("Client: server ip address: %s and port: %d \n", host_addr, port);
	if (!(host = gethostbyname (host_addr)))
	{
		perror ("could not obtain host address as");
		exit (2);
	}

	printf("Client: Opening file: %s in %s mode \n", filename, (opcode == WRQ ? "Read": "Write") );
	if(opcode == WRQ)
		fp = fopen (filename, "r");	
	else if(opcode == RRQ)
		fp = fopen (filename, "w");		
	if (fp == NULL)
	{
		printf ("Client: file could not be opened: file not found or permission denied\n");
		return 0;
	}
	fclose (fp);		


	/*Create the socket, a -1 will show us an error */
	if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
	{
		perror("Client: socket");
		return 0;
	}

	 // bind to an arbitrary return address 
	 // because this is the client side, we don't care about the 
	 // address since no application will connect here  
	 // INADDR_ANY is the IP address and 0 is the socket 
	 // htonl converts a long integer (e.g. address) to a network 
	 // representation (agreed-upon byte ordering 

	bzero(&server, sizeof (server));
	server.sin_family = AF_INET;
	memcpy (&server.sin_addr, host->h_addr, host->h_length);
	// server.sin_addr.s_addr = htonl (INADDR_ANY);
	server.sin_port = htons (port);	


	len2 = sizeof(server);
	memset(buf, 0, 512);
	// creates RRQ packet
	len = request_packet(opcode, filename, mode, buf);
	if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, len2) != len)
	{
		perror("Client: sendto has returend an error");
		exit(-1);
	}
	printf("Client: %s packet sent\n", (opcode==1 ? "RRQ":"WRQ"));
	switch (opcode)
	{
		case RRQ:
			client_get(filename, server, mode, sock);
			break;
		case WRQ:
			client_put(filename, server, mode, sock);
			break;
		default:
			printf("Invalid opcode. Packet discarded.");
	}
	close(sock);
	return 1;
}