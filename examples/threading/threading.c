#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    thread_func_args->thread_complete_success = false;
  
    // TODO-DONE: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    /* wait */
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    
    /* obtain mutex */
    int rc = pthread_mutex_lock( thread_func_args->mutex);
    if (0 == rc) {
      /* wait */
      usleep(thread_func_args->wait_to_release_ms * 1000);
      /* release mutex */
      rc = pthread_mutex_unlock( thread_func_args->mutex);
    }  

    thread_func_args->thread_complete_success = !(bool) rc;

    return thread_param;
}

bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data 	*thread_func_args;
    /**
     * TODO-DONE: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

     /* allocate memory for thread_func_args*/
    thread_func_args = malloc(sizeof(*thread_func_args));
     
     /* setup mutex and wait arguments */
    thread_func_args->wait_to_obtain_ms = wait_to_obtain_ms;
    thread_func_args->wait_to_release_ms = wait_to_release_ms;
    thread_func_args->mutex = mutex;

    /* create thread and pass data */
    return !(bool) pthread_create( thread, NULL, &threadfunc, thread_func_args);
}

