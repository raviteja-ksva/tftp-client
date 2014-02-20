// Defined Opcodes
#define ERROR -1
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 0x04
#define ERR 0x05

#define TIMEOUT 1000		// timeout value is 1000 ms = 1 sec.
#define RETRY 4			// # of times to resend a data OR ack packet beforing suspending

static char buf[512];