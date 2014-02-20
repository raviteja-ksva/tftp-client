#include <stdio.h>		
#include <string.h>		
#include <stdlib.h> 	
#include <sys/types.h>	
#include <sys/socket.h>	
#include <netdb.h>		
#include <netinet/in.h>
#include <unistd.h>

#include "fill_headers.h"
#include "client_get.h"
#include "client_put.h"
#include "tftp.h"

int port = 69;

int main (int argc, char **argv)
{
	int sock, server_len, len, opt;
	char opcode, filename[196], mode[12] = "octet";
	struct hostent *host;
	struct sockaddr_in server;
	char* host_ip = "127.0.0.1";
	FILE *fp;
	if (argc < 2)
	{
		printf("invalid format of command\n");
		printf("Command format: ./client -h [host_ip] -P [port] -g|-p [filename] [-n] \n");
		return 0;
	}

	while ((opt = getopt(argc, argv, "h:P:p:g:n")) != -1)
	{
		// printf("inside first while \n");
		switch (opt)
		{
			case 'h':
				host_ip = optarg;
				// printf("Client: server ip is %s\n", host_ip);
				break;			
			case 'P':		// get port number
				port = atoi (optarg);
				// printf ("Client: The port number is: %d\n", port);
				break;
			case 'p':
//				printf("need to put a file in server\n\n");
				strncpy (filename, optarg, sizeof (filename) - 1);
				opcode = WRQ;
				break;

			case 'g':
				strncpy (filename, optarg, sizeof (filename) - 1);
				opcode = RRQ;
				break;

			case 'o':
				strncpy (mode, "octet", sizeof (mode) - 1);
				printf ("Client: The mode is set to octet\n");
				break;
			case 'n':
				strncpy (mode, "netascii", sizeof (mode) - 1);
				printf ("Client: The mode is set to netascii\n");
				break;
			default:
				// printf("invalid format of command\n");
				printf("Command format: ./client -h [host_ip] -P [port] -g|-p [filename] [-n] \n");
				return (0);
				break;
		}
	}
	printf("Client: server ip address: %s and port: %d \n", host_ip, port);
	if (!(host = gethostbyname (host_ip)))
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


	server_len = sizeof(server);
	//printf("value os BUFSIZ is %d \n", BUFSIZ);
	memset(buf, 0, BUFSIZ);
	// creates RRQ packet
	len = req_packet(opcode, filename, mode, buf);
	if (sendto (sock, buf, len, 0, (struct sockaddr *) &server, server_len) != len)
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