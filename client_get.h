#include "tftp.h"

/*a function to get a file from the server*/
void tget (char *Filename, struct sockaddr_in server, char *Mode, int sock);