#include <stdio.h>
#include "ipc_lp_sim.h"

#define         LPNAME       "LP2"
#define         NEIGHBOUR1   "LP3"
#define         NEIGHBOUR2   "LP4"

double          end_clock = 100.0;
gboolean        end_reached = FALSE;
GQueue        * queue;
MsgHeaderIPC  * msg;


double
rnd (int min, int max) {
   return (double)((random()%(max-(min-1)))+min);
}

char *
rnd_dest (void) {
   char  * name;

   name = (char *)malloc (sizeof(char)*16);
   if ( (random()%2) == 0 )
      strcpy (name, NEIGHBOUR1);
   else
      strcpy (name, NEIGHBOUR2);
   return name;
}


int main(int argc, char* argv[])
{ 
   int      i=1;
   double   last_ts = 0.0, ts;
   char     lmsg[24], dest[15];
   
   srandom (getpid());
   init_communication (LPNAME);
	
   while (!end_reached) {
      queue = receive_event ();
      if (queue != NULL) {
         msg = (MsgHeaderIPC *)g_queue_pop_head (queue);
         clock = msg->time_stamp;
         if (clock >= end_clock) {
             printf ("[%s]: -- end condition reached, clock=%.2f --\n", LPNAME, clock);
             end_reached = TRUE;
         } else {
            strncpy (dest, rnd_dest(), 14);
            printf ("[%s]: Ricevuto da %s, '%.2f %s'\n", 
                      LPNAME, msg->target, msg->time_stamp, msg->msg);
			ts = clock + (strcmp(dest,NEIGHBOUR1)==0 ? 0.2+rnd(0,2) : 0.4+rnd(0,2));
            schedule_extern_event (ts, dest, msg->msg);
            printf ("[%s]:   Schedulato '%.2f %s' per %s\n", LPNAME, ts, msg->msg, dest);
         }
         g_queue_free (queue);         
      }
      sim_V ();
   }
   
   close_communication ();
}
