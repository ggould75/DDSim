#include <stdio.h>
#include "ipc_lp_sim.h"

#define         LPNAME       "PALERMO"
#define         NEIGHBOUR1   "MILANO"
#define         NEIGHBOUR2   "ROMA"

typedef struct _sub_msg_header {
   char   event_type[4];
   char   from[20];
} SubMsgHeader;

double          end_clock = 150.0;
gboolean        end_reached = FALSE;
GQueue        * queue;
MsgHeaderIPC  * msg;
SubMsgHeader  * submsg;

/* simulator state variables */
gboolean        runwayfree;
int             on_the_ground;
int             in_the_air;


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

char *
create_submsg (char type[], char from[]) {
   SubMsgHeader  * tmsg;
   tmsg = (SubMsgHeader *)malloc (sizeof(SubMsgHeader));
   memset (tmsg, 0, sizeof(SubMsgHeader));
   strcpy (tmsg->event_type, type);
   strcpy (tmsg->from, from);
   return (char *)tmsg;
}


void
arrival_event_handler (void) {
   printf ("[%s]:   %.1f  In attesa di atterrare volo da %s\n", 
             LPNAME, msg->time_stamp, msg->target);
   in_the_air++;
   if (runwayfree) 
      runwayfree = FALSE;
   schedule_local_event (clock + rnd(0,2), create_submsg ("LAN", msg->target));
}

void
landed_event_handler (void) {
   printf ("[%s]:     %.1f  Atterrato volo da %s\n", 
             LPNAME, msg->time_stamp, submsg->from);
   in_the_air--;
   on_the_ground++;
   schedule_local_event (clock + rnd(0,1), create_submsg ("DEP", ""));
   if (in_the_air <= 0)
      runwayfree = TRUE;
}

void
departure_event_handler (void) {
   double  ts;
   char    dest[15];
   
   on_the_ground--;
   strncpy (dest, rnd_dest(), 14);
   ts = clock + (strcmp(dest,NEIGHBOUR1)==0 ? 0.7+rnd(0,1) : 0.3+rnd(0,1));  // 7,9 - 0.1+r(0,1)
   schedule_extern_event (ts, dest, create_submsg ("ARR", ""));
   printf ("[%s]: %.1f  Partito volo per %s\n",  
             LPNAME, msg->time_stamp, dest);
}


int main(int argc, char* argv[])
{ 
   srandom (getpid());
   init_communication (LPNAME);
   runwayfree = TRUE;
   on_the_ground = 1;
   in_the_air = 0;
	
   sim_P ();
   schedule_local_event (clock+rnd(0,5), create_submsg ("DEP", ""));
   sim_V ();
	
   while (!end_reached) {
       queue = receive_event ();
       if (queue != NULL) {
          msg = (MsgHeaderIPC *)g_queue_pop_head (queue);
          submsg = (SubMsgHeader *)msg->msg;
          clock = msg->time_stamp;

          if (clock <= end_clock) { 
             if (strcmp (submsg->event_type, "ARR")==0)  arrival_event_handler ();
             if (strcmp (submsg->event_type, "LAN")==0)  landed_event_handler ();
             if (strcmp (submsg->event_type, "DEP")==0)  departure_event_handler ();
          } else {
             printf ("[%s]: -- condizione arresto raggiunta, clock=%.2f\n", LPNAME, clock);
             end_reached = TRUE;
          }
          g_queue_free (queue);
       }
       sim_V ();
   }
	
   close_communication ();
}
