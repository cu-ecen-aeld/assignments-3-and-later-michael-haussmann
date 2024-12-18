#include <syslog.h>
#include <stdio.h>

int main(int argc, char * argv[]) {

  FILE *fptr; /* pointer to the file, we shall write to */

  /* open syslog, so we can write our output there */
  openlog("writer", LOG_PERROR, LOG_USER);

  /* we expect 2 command line parameters, but keep in mind: argv[0] is the command
    itself, so we check for 3 argv's!*/
  if (3 != argc) {
    syslog( LOG_ERR, "Wrong number of argument");
    return(1); 
  }
  
  fptr = fopen(argv[1], "w");
  if (NULL == fptr) {
    syslog( LOG_ERR, "Could not open file: %s", argv[1]);
    return(1);
  }

  syslog( LOG_DEBUG, "write to syslog: Writing %s to %s\n", argv[2], argv[1]);
  fprintf( fptr, "%s", argv[2]);
  
  fclose(fptr); /* you better close the file on exit ;-) */
  
}