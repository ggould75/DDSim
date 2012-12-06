#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "ipc_lp_sim.h"

//#define DEBUG   1;

int       fifo_fdin;
int       fifo_fdout;


/**
 * Prints an error and exits
 */
static void
error_exit (const char *str) {
    char strmess[1024];
	
    sprintf ((char *)strmess, "%s, Err:%d \"%s\"\n", str,
                                                     errno, 
                                                     strerror(errno));
    fprintf (stderr, "%s", strmess);
    fflush (stderr);
    exit (1);
}


/**
 * Creates a fifo for the communications with its LP, init the clock to 0,
 * setup the name of the local LP (used in schedule_local_event())
 */
void
init_communication (char this_lp_name[]) {
    char tmp[25];
	
    printf("*** simulator %s ready ***\n", this_lp_name);
	
    sprintf (tmp, "to-%d", getpid());	
    mkfifo(tmp, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

    if ( (fifo_fdin = open(tmp, O_RDONLY)) < 0)
       error_exit ("open fifoin error");
	
	sprintf (tmp, "to-p-%d", getppid());
	if ( (fifo_fdout = open(tmp, O_WRONLY)) < 0)
       error_exit ("open fifoout error");

	clock = 0.0;
    strcpy (local_lp_name, this_lp_name);
}


/**
 * Closes the fifo with the LP in a syncronized way, this avoid errors such as 'broken pipe'
 */
void
close_communication (void) {
   char  type;

   do {
      if ( read (fifo_fdin, &type, sizeof(char)) < 0 )
         error_exit ("close_communication() read error:");
	 
      if (type == END_MSG)
         if ( write (fifo_fdout, &type, sizeof(char)) < 0 )
            error_exit ("close_communication() write error:");
	  
   } while (type != CLOSE_MSG);
	  
   if ( write (fifo_fdout, &type, sizeof(char)) < 0 )
      error_exit ("close_communication() write error:");

   close (fifo_fdin);
   close (fifo_fdout);
}


/**
 * Requests to its LP to send a new event
 */
void
schedule_extern_event (double time_stamp, char dest[], char msg[]) {
   MsgHeaderIPC  msgh;

   memset (&msgh, 0, sizeof(MsgHeaderIPC));
   msgh.type = TRUE_MSG;
   msgh.time_stamp = time_stamp;
   strcpy (msgh.target, dest);
   memmove (msgh.msg, msg, 24);
	
   if ( write (fifo_fdout, &msgh.type, sizeof(msgh.type)) < 0 )
      error_exit ("schedule_extern_event() write error:");
   if ( write (fifo_fdout, &msgh.time_stamp, sizeof(msgh.time_stamp)) < 0 )
      error_exit ("schedule_extern_event() write error:");
   if ( write (fifo_fdout, &msgh.target, sizeof(msgh.target)) < 0 )
      error_exit ("schedule_extern_event() write error:");
   if ( write (fifo_fdout, &msgh.msg, sizeof(msgh.msg)) < 0 )
      error_exit ("schedule_extern_event() write error:");
  #ifdef DEBUG
   printf ("spedito '%.2f %s' a %s\n", time_stamp, msgh.msg, msgh.target);
  #endif
}


/**
 * Convenience function like the previous using the name of the local LP
 */
void
schedule_local_event (double time_stamp, char msg[]) {
   schedule_extern_event (time_stamp, local_lp_name, msg);
}


GQueue *
receive_event (void) {
   int             res;
   char            type;
   GQueue        * msg_queue = NULL;
   gboolean        end = FALSE;
   MsgHeaderIPC  * msgh_ipc;
   
   while (!end) {
       if ( read (fifo_fdin, &type, sizeof(type)) < 0 )
           error_exit ("receive_extern_event() msgh_ipc1 read error:");
       switch (type) {
          case TRUE_MSG:
             msgh_ipc = (MsgHeaderIPC *)malloc (sizeof(MsgHeaderIPC));
             msgh_ipc->type = type;
			  
             if ( read (fifo_fdin, &msgh_ipc->time_stamp, sizeof(msgh_ipc->time_stamp)) < 0 )
                error_exit ("receive_extern_event() msgh_ipc-1 read error:");
             if ( read (fifo_fdin, msgh_ipc->target, sizeof(msgh_ipc->target)) < 0 )
                error_exit ("receive_extern_event() msgh_ipc-2 read error:");
             if ( read (fifo_fdin, msgh_ipc->msg, sizeof(msgh_ipc->msg)) < 0 )
                error_exit ("receive_extern_event() msgh_ipc-3 read error:");

             msg_queue = g_queue_new();    			  
             g_queue_push_tail (msg_queue, (gpointer)msgh_ipc);
             break;
          
          // The LP has not sent any processable msg --> syncronize the clock.
          // In this way I will be able to generate new events with the right clock
          case SYNC_MSG:
             if ( read (fifo_fdin, &clock, sizeof(double)) < 0 )
                error_exit ("receive_extern_event() clock read error:");
             end = TRUE;
             break;
          
          // ...That’s all I have to say about that... (F.G.)
          case END_MSG: 
             end = TRUE;
       }
   }
   return msg_queue;
}


/**
 * Wait until the LP say: "Go with god".
 * The clock is also syncronized if necessary
 */
void 
sim_P (void) {
   char  type;

   if ( read (fifo_fdin, &type, sizeof(char)) < 0 )
       error_exit ("sim_P() read error:");
   if (type == SYNC_MSG) {
      if ( read (fifo_fdin, &clock, sizeof(double)) < 0 )
         error_exit ("sim_P() clock read error:");
   }
}

/**
 * Gives back the ownership to the LP
 */
void 
sim_V (void) {
   char  end_char;

   end_char = END_MSG;
   if ( write (fifo_fdout, &end_char, sizeof(char)) < 0 )
       error_exit ("sim_V() write error:");
}

