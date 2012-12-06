#include <stdio.h>
#include "ipc_lp_sim.h"
 
#define         LPNAME       "LP1"
#define         NEIGHBOUR1   "LP2"

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
	
   sim_P ();
   last_ts = clock;
   do {
      ts = last_ts+rnd(0,2);
      last_ts = ts;
      sprintf (lmsg, "msg%d", i);
      printf ("[%s]: local event scheduled %.2f\n", LPNAME, ts);
      schedule_local_event (ts, lmsg);
      i++;
   } while (last_ts < end_clock);
   sim_V ();
   
   while (!end_reached) {
      queue = receive_event ();
      if (queue != NULL) {
         msg = (MsgHeaderIPC *)g_queue_pop_head (queue);
         clock = msg->time_stamp;
         if (clock >= end_clock) {
            printf ("[%s]: -- end condition reached, clock=%.2f --\n", LPNAME, clock);
            end_reached = TRUE;
         } else {
            ts = clock + 0.5 + rnd(0,1);
            schedule_extern_event (ts, "LP2", msg->msg);
            printf ("[%s]: Scheduled '%.2f %s' for LP2\n", LPNAME, ts, msg->msg);
         }
         g_queue_free (queue);
      }
      sim_V ();
   }
   
   close_communication ();
}
