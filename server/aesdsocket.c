#include "aesdsocket.h"
#include "daemonize.h"

/* global variables */
bool relevant_signal_caught;

/* ============= Print the found ip-addreses to syslog, if debug is defined ======================== */
#ifdef debug
void logipaddresses( bool enumerate, char *text, struct addrinfo *my_addrinfo) {
  struct addrinfo *p; /* returned addrinfo by getaddrinfo */
  /* Loop through results and print out IP addresses */
  #define IPSTRLEN INET6_ADDRSTRLEN 
  char ipstr[IPSTRLEN];
  for(p = my_addrinfo; p != NULL; p = p->ai_next) {

    void *addr;  
    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
    } 
    else { // IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
    }

    inet_ntop(p->ai_family, addr, ipstr, IPSTRLEN);
    syslog( LOG_INFO, "%s %s\n", text, ipstr);
    
    if (!enumerate) break;
  }  
  
  return;
}
#endif

/* ============= Signal handler ======================== */
static void signal_handler( int signal_number) {
  relevant_signal_caught = false;
  switch (signal_number) {
    case SIGINT: 
    case SIGTERM: {
      relevant_signal_caught = true; 
      break;
    }
    default: 
  }
  return;
}

/* ============= Convert a struct sockaddr address to a string for IPv4 and IPv6: ==================== */
void get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch(sa->sa_family) {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
    }

    return;
}

/* ============= Data handler ========================== */
int data_handler (int my_sock, struct sockaddr clientaddr) {
  /* this function can run as a "normal function" (called directly from the app or
   * as a part of the daemon 
  */ 

  int handler_result = -1;
  int filefd;					/* file descriptor for the communication buffer file */
  int sockfd;					/* file decriptor for data transmission (accept) */  
  socklen_t addrlen = sizeof(clientaddr); 	/* length of clientaddr */   
  ssize_t bytes_received;
  ssize_t bytes_sent;
  char *buffer_received;
  char *clientipstr;
  
  /* open the file we want to write the received messages to. file is being created, if not existing, rw rights for owner only*/
  filefd = open( combuffile, O_APPEND | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
  if(-1 == filefd) {
    #ifdef debug
    syslog( LOG_ERR, "data handler failed opening/creating file %s", combuffile);
    #endif
    goto exit_handler;  
  }
  
  // preparing the buffer for communication (a string)....
  buffer_received = malloc(MAX_MESSAGE_LEN+1);
  if (NULL == buffer_received) {
    #ifdef debug
    syslog( LOG_ERR, "data handler could not malloc buffer for %i bytes", MAX_MESSAGE_LEN+1);
    #endif
    goto exit_handler;
  }

  #define MAX_IP_STR_LEN 255
  clientipstr = malloc( MAX_IP_STR_LEN + 1);
  
  while(1) { // loop accepting new connections (we will close with SIGINT or SIGTERM)

    // accept socket connection. This is blocking, we need to take care about signals!
    #ifdef debug
    syslog(LOG_INFO, "Waiting for connection on port %s", listenport);
    #endif
    sockfd = accept( my_sock, &clientaddr, &addrlen);  /* this is blocking! */
    if (relevant_signal_caught) {
      handler_result = 0; goto exit_handler;
    } else if (-1 == sockfd) {
      #ifdef debug
      syslog( LOG_ERR, "data handler: error in accpet (connection)");
      #endif
      goto exit_handler;
    } // else, we have a file descriptor...

    get_ip_str( &clientaddr, clientipstr,  MAX_IP_STR_LEN -1); 
    syslog( LOG_INFO, "Accepted connection from %s", clientipstr);    
  
    /* use recv and send to actually transfer data (we're allowed to use blocking or non-blocking recv! */
    bytes_received = recv( sockfd, buffer_received, MAX_MESSAGE_LEN, 0);  /* this is blocking! */
    buffer_received[bytes_received] = 0;
    if (relevant_signal_caught) {
      handler_result = 0; goto exit_handler;      
    } else if ( 0 >= bytes_received) {
      #ifdef debug
      syslog( LOG_ERR, "data handler received 0 or less (error) bytes -> should not occur!");
      #endif
      goto exit_handler;
    }  else {
      #ifdef debug    
      syslog( LOG_INFO, "received %ld bytes", bytes_received);
      #endif
      
      // we don't need to check that the file pointer is at the end
      // write buffer_recevied to file (append)
      if (bytes_received != write( filefd, buffer_received, bytes_received)) {
        #ifdef debug
        syslog( LOG_ERR, "data handler error writing to file: %s", combuffile);
        #endif
        goto exit_handler;
      }
      // - read all strings from file and send them 
      if (-1 == lseek(filefd, 0, SEEK_SET)) {
        #ifdef debug
        syslog( LOG_ERR, "data handler setting file pointer to 0 in file: %s", combuffile);
        #endif
        goto exit_handler;
      }
      do {
        bytes_received = read( filefd, buffer_received, MAX_MESSAGE_LEN);
        #ifdef debug
        syslog(LOG_INFO, "read %ld bytes from file", bytes_received);
        #endif
        if(-1 == bytes_received) {
          #ifdef debug
          syslog( LOG_ERR, "data handler error reading from file: %s", combuffile);
          #endif
          goto exit_handler;
        }

        if(bytes_received != 0) {
          buffer_received[bytes_received+1] = 0;
          #ifdef debug
          syslog( LOG_INFO, "sending %ld bytes", bytes_received);        
          #endif
          bytes_sent = send(sockfd, buffer_received, bytes_received, 0);
          if( bytes_sent != bytes_received) {        
            #ifdef debug
            syslog( LOG_ERR, "data handler error %ld bytes on socket: %i", bytes_received, sockfd);
            #endif
            goto exit_handler;      
          }   
        }
      }  while (bytes_received != 0);

      syslog(LOG_INFO, "Closed connection from %s", clientipstr);
      close(sockfd);      
    }
  }

exit_handler:
  free(clientipstr);    
  free(buffer_received);
  fclose(stdin);
  fclose(stdout);
  fclose(stderr);
  close(filefd);
  remove(combuffile);
  return (handler_result);
}

/* ============= Main function  ======================== */
int main(int argc, char * argv[]) {

/* */

/* TO-DO:
    Open a stream socket bound to port 9000, failing and returning -1 if any of the socket connection steps fail.
    Listen for and accept a connection
    Log message to the syslog “Accepted connection from xxx” where XXXX is the IP address of the connected clientl. 
    Receive data over the connection and append to file /var/tmp/aesdsocketdata, create this file if it doesn’t exist.
    Return the full content of /var/tmp/aesdsocketdata to the client as soon as the received data packet completes.
    Restart accepting connections from new clients forever in a loop until SIGINT or SIGTERM is received 
    Gracefully exit when SIGINT or SIGTERM is received, complete any open connection operations, 
      close any open sockets, and delete the file /var/tmp/aesdsocketdata
      - consider using shotdown() in the signal hanlder strategy!
    Support a -d argument which runs the aesdsocket application as a daemon. 
      When in daemon mode the program should fork after ensuring it can bind to port 9000.
      
      
      We should use valgrind:
      valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=/tmp/valgrind-out.txt ./aesdsocket
*/

  int retval = -1; 	 	/* program returns -1 in case an error occured */
  #ifdef debug
  int myerrno = 0;		/* in case something goes wrong, we want to save the errno */
  #endif

  struct sigaction new_action;  /* signal action to redister signal handler*/
  int logfacility = LOG_USER;   /* facility for syslog (LOG_DAEMON or LOG_USER (default)*/
  bool daemonmode = false;      /* are we in daemon mode or not? */
  struct addrinfo hints; 	      /* hints for getaddrinfo*/
  struct addrinfo *result, *rp; /* returned addrinfo by getaddrinfo */
  struct sockaddr clientaddr;	  /* information about the connected client */  
  
  int my_sock;			    /* socket used for receive/transmit */
  int reuseaddr = 1;		/* enable SO_REUSEADDR in socket-options (setsockopt) */

  // check, if we shall enter the daemon-mode
  for( int i=0; i < argc; ++i) {
    if (0 == strcmp("-d", argv[i] )) {
      daemonmode = true;
      logfacility = LOG_DAEMON; /* log will be (re)opened by daemonze, but we want to log as daemon from the very beginning*/
      break;
    }
  }  
  // open the syslog
  openlog("aesdsocket", LOG_PID, logfacility);

// let syslog know, we've started
  if(daemonmode) {
    syslog(LOG_INFO, "========== started in DAEMON-mode ==========");
  } else {
    syslog(LOG_INFO, "========== started in USER-mode ==========");
  }

  // register the signal-handler
  memset(&new_action,0,sizeof(struct sigaction));
  new_action.sa_handler=signal_handler;
  if( sigaction(SIGTERM, &new_action, NULL) != 0 ) {
    #ifdef dcebug
    syslog(LOG_ERR, "Error %d (%s) registering for SIGTERM",errno,strerror(errno));
    #endif
    goto exit_aesdsocket;
  }
  if( sigaction(SIGINT, &new_action, NULL) != 0) {
    #ifdef debug
    syslog(LOG_ERR, "Error %d (%s) registering for SIGINT",errno,strerror(errno));
    #endif
    goto exit_aesdsocket;
  }

  // set the hints for getaddrinfo --> all this is non-blocking, no need to check for signals
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;  	/* don't care, if IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;	/* TCP, not UDP! */
  hints.ai_flags = AI_PASSIVE;		/* server socket -> wait for connection (opposite of !AI_PASSIVE is used for connect() */
  
  if (0 != getaddrinfo( NULL, listenport, &hints, &result)) {
    #ifdef debug
    syslog(LOG_ERR, "getaddrinfo failed");
    #endif
    goto exit_aesdsocket; // error...
  }
  #ifdef debug 
  logipaddresses(true, "getaddrinfo found address: ", result); 
  #endif

  /* bind the socket --> non-blocking, no need to check for signals!
     getaddrinfo() returns a list of address structures.
     Try each address until we successfully bind(2).
     If socket(2) (or bind(2)) fails, we (close the socket
     and) try the next address. */
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    my_sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (my_sock == -1) continue; // socket failed

    setsockopt(my_sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr));
    if (bind(my_sock, rp->ai_addr, rp->ai_addrlen) == 0) {  
      #ifdef debug
      logipaddresses(false, "bind successfull on IP: ", result); 
      #endif
      break;                  /* Success */
    }
    #ifdef debug
    myerrno = errno;
    syslog( LOG_ERR, "bind failed with errno %i (%s)", myerrno, strerror(myerrno));       
    #endif
    close(my_sock);
  }
       
  if (NULL == rp) {  /* no address succeded, leave the program, retun -1 */
    #ifdef debug
    syslog( LOG_ERR, "bind failed with all addresses");
    #endif
    goto exit_aesdsocket; // error...
  }

  /* don't forget to freeaddrinfo after we have processed the addrinfo -> it's malloc'ed! */  
  freeaddrinfo(result);
  /* listen on the socket previously created and bind*/
  #ifdef debug
  syslog(LOG_INFO, "Listen on port: %s", listenport);
  #endif
  if ( 0 !=listen(my_sock, 1)) {
    goto exit_aesdsocket; // error...
  }

// TO-DO: here, we have to fork (daemon mode), when option -d was set
  if (daemonmode) { 
    int ports_open [] = {my_sock}; // daemonize will close all file descriptor except my_sock
    if ( 0!= daemonize("aesdsocket", "", NULL, NULL, NULL, ports_open, 1)) {
    syslog(LOG_ERR, "daeominze failed, exiting.");  
    goto exit_aesdsocket;
    }
  }

  // start the data-handler (either directly or as daemon - which daemonize did ;-)
  retval = data_handler( my_sock, clientaddr); 
  if( 0 == retval) { 
    // data_handler will run forever, except one of the relevant signals is received, so 0 only in this case
    syslog( LOG_INFO, "Caught signal, exiting");
  } else {
    #ifdef debug
    syslog( LOG_ERR, "data handler terminated with error %i", retval);
    #endif
  }
  
/* =================== the end... =======================*/
exit_aesdsocket:
// TO-DO: close my_sock and fd (file descriptor!
  #ifdef debug
  syslog(LOG_INFO, "exit with code %i", retval);
  #endif
  close(my_sock);   
  closelog();
  return(retval);
}