#include "systemcalls.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "wait.h"
#include "errno.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{
/*
 * implemented according to assignment 3 - part 1
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    int res = system(cmd);
    if(-1==res) {
      return (false);
    } else {
      return (true);
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    bool res = false; /* I want to avoid more than 1 return (MISRA ;-)) */
    pid_t pid;
    int status;
    pid_t waitret; 

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
/*
 *  implemented according to assignment 3 - part 1
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    pid = fork();
    if ( 0 == pid) { /* we are the child */
      /* we do not need to check the return value of execv: either it returns with -1 (which is a failure. Remember: res is set to 0) or it does not return 
         if it returns, we exit with errno!*/
      execv( command[0], command);
      exit(errno);
    } else if (0 < pid) { /* we are the parent */
      waitret = wait(&status);
      if((-1 != waitret) && (WIFEXITED(status)) && (0 == WEXITSTATUS(status))) {
        res = true; 
      }
    }

    va_end(args);

    return (res);
}


/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    bool res = false; /* to avoid more than one return */
    pid_t pid;
    int fd;
    int status;
    int reterrno;
    pid_t waitret;

    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;

/*
 *  implemented according to assignment 3 - part 1
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/

    /* do the same as in do_exec() */
    /* note: in a real world, I would call the function instead of replicating the code here */

    pid = fork();
    if ( 0 == pid) { /* we are the child */
      /* redirect stdout to outputfile*/
      fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
      if (0 < fd) {
        if (0 < dup2(fd, 1)) {    
          /* we do not need to check the return value of execv: either it returns with -1 (which is a failure: remember: res is set to false) or it does not return */
          execv( command[0], command);     
          reterrno = errno; /* but in case of error, we need to return errno to the parent! First, save it!*/
          close(fd);        /* then close the fd (attention! this can change errno, but we're good, we saved it! */
          exit(reterrno);   // and return errno to the parent
        }
      }
      
    } else if (0 < pid) { /* we are the parent */
      waitret = wait(&status);
      if ((-1 != waitret) && (WIFEXITED(status)) && (0 == WEXITSTATUS(status))) { res = true; }
    }

    va_end(args);

    return (res);
}
