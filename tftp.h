// Defined Opcodes
#define ERROR -1
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 0x04
#define ERR 0x05

#define TIMEOUT 1000		// timeout value is 1000 ms = 1 sec.
#define RETRIES 3			// # of times to resend a data OR ack packet beforing suspending
#define MAXDATASIZE 1024  	// Maximum data size allowed
#define EAGAIN 11

/*a buffer for the messages passed between the client and the server */
static char buf[BUFSIZ];
// static int debug;
// int errno;