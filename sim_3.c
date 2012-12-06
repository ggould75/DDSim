#include <stdio.h>
#include "ipc_lp_sim.h"

#define         LPNAME       "LP3"
#define         NEIGHBOUR1   "LP5"
 
double          end_clock = 100.0;
gboolean        end_reached = FALSE;
GQueue        * queue;
MsgHeaderIPC  * msg;


double
rnd (int min, int max) {
   return (double)((random()%(max-(min-1)))+min);
}


int main(int argc, char* argv[])
{ 
   int      i=1;
   double   last_ts = 0.0, ts;
   char     lmsg[24];
   
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
            printf ("[%s]: Received from %s, '%.2f %s'\n", 
                      LPNAME, msg->target, msg->time_stamp, msg->msg);
            ts = clock + 0.2 + rnd(0,2);
            schedule_extern_event (ts, NEIGHBOUR1, msg->msg);
            printf ("[%s]:   Scheduled '%.2f %s' for %s\n", LPNAME, ts, msg->msg, NEIGHBOUR1);
         }
         g_queue_free (queue);
      }
      sim_V ();
   }
   
   close_communication ();
}
