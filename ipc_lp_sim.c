#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "ipc_lp_sim.h"

//#define DEBUG   1;

int       fifo_fdin;
int       fifo_fdout;


/* print error msg and exit */
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


/* Crea fifo per comunicare con il proprio LP, inizializza clock a 0,
 * imposta il nome dell'LP locale (usato in schedule_local_event()) */
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

/* chiude in modo sincronizzato la fifo con l'LP,
 * questo evita errori di 'broken pipe' */
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

/* Spedisce al proprio LP una richiesta di invio messaggio ad LP 'dest' */
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


/* E' un "alias" alla funzione precedente che passa pero' come dest.
 * il nome dell'LP locale */
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
          
          /* l'LP non ha inviato alcun msg processabile--> devo sincronizzare il clock,
		   * in questo modo se voglio generare nuovi eventi lo faccio usando il clock giusto !!! */
          case SYNC_MSG:
             if ( read (fifo_fdin, &clock, sizeof(double)) < 0 )
                error_exit ("receive_extern_event() clock read error:");
             end = TRUE;
             break;
          
          /* ...non c'e' altro da dire su questa faccenda... (f.g.) */
          case END_MSG: 
             end = TRUE;
       }
   }
   return msg_queue;
}

/* attende che LP dica: "vai con DIO".
 * sincronizza anche il clock se necessario */
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

/* restituisce il controllo a LP */
void 
sim_V (void) {
   char  end_char;

   end_char = END_MSG;
   if ( write (fifo_fdout, &end_char, sizeof(char)) < 0 )
       error_exit ("sim_V() write error:");
}

