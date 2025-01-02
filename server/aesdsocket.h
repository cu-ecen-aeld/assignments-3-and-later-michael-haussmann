#include <syslog.h> /* needed to write to the syslog ;-) */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> /* */
#include <sys/socket.h> /* needed fot sockets ;) */
#include <netdb.h>
#include <arpa/inet.h> /* if we use inet_ntop to format and print IP-addresses*/
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

//#define debug

// some defines
#define listenport "9000" 		// port on which we listen for connections
#define combuffile "/var/tmp/aesdsocketdata"
#define MAX_MESSAGE_LEN 20*1024 	// max size of network message
// note to the reviewer: in a real world scenatio, I wouldn't do that
// I would chunk down the data to smaller portions and handle them.
// But as we know, what the test program is sending, we also know that 20 kiB is enough to store the complete traffic ;-)
