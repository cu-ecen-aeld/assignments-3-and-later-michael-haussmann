#include "daemonize.h"

/* ==================== forks the app to run as daemon =========================*/
/* - forks and detaches a daemon thread
   --> uses the "double for technique, see here:
       https://0xjet.github.io/3OHA/2022/04/11/post.html
   - changes directory to "path" (defaul: "/") to avoid that we block a mount-point
   - redirects stdin, stdout and stderr to the given files (default: /dev/null) 
   ATTENTION: the app itself needs to implement signal handlers!  
*/

int daemonize(char* name, char* path, char* outfile, char* errfile, char* infile, const int fd_except[], size_t fd_except_size)
{
  bool closeport = true;

  if(!path) { path="/"; }
  if(!name) { name="medaemon"; }
  if(!infile) { infile="/dev/null"; }
  if(!outfile) { outfile="/dev/null"; }
  if(!errfile) { errfile="/dev/null"; }

  pid_t child;
  //fork, detach from process group leader
  if( (child=fork())<0 ) { //failed fork
    syslog(LOG_ERR, "daemonize error: 1. fork failed");
    exit(EXIT_FAILURE);
  }
  if (child>0) { //parent
    exit(EXIT_SUCCESS);
  }
  if( setsid()<0 ) { //failed to become session leader
    syslog(LOG_ERR,  "daemonize error: setsid failed");
    exit(EXIT_FAILURE);
  }

  //catch/ignore signals is left to the application!

  //fork second time
  if ( (child=fork())<0) { //failed fork
    syslog(LOG_ERR, "error: 2. fork failed");
    exit(EXIT_FAILURE);
  }
  if( child>0 ) { //parent
    exit(EXIT_SUCCESS);
  }

  //new file permissions
  umask(0);
  //change to path directory
  chdir(path);

  // Close all open file descriptors, except those in the exception list (array)
  for( int fd=sysconf(_SC_OPEN_MAX); fd>=0; --fd )
  {
    closeport = true;
    for (size_t except_port=0; except_port < fd_except_size; ++except_port) {
      closeport = false;
      break;
    }
    if (closeport) close(fd);
  }

  //reopen stdin, stdout, stderr
  stdin=fopen(infile,"r");     //fd=0
  stdout=fopen(outfile,"w+");  //fd=1
  stderr=fopen(errfile,"w+");  //fd=2

  //(re)open syslog
  openlog(name,LOG_PID,LOG_DAEMON);
  return(0);
}
