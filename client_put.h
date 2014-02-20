#include "tftp.h"

/*a function to send a file to the server*/
void tsend (char *pFilename, struct sockaddr_in server, char *pMode, int sock);