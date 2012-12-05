#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "msg_header_ipc.h"
#include "channels.h"


gboolean    new_msg;
gboolean    end_condition_reached;
pid_t       pid;
char        child_fifoname[25];
char        parent_fifoname[25];

void
load_simulator (void) {
   sprintf (parent_fifoname, "to-p-%d", getpid());
   if (mkfifo(parent_fifoname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) < 0)
     error_exit ("parent MKFIFO error");
	
   if ( (pid = fork ()) < 0 ) error_exit ("fork error");
     else 
        if (pid == 0) { /* child */
            char tmp[25];			
            sprintf (tmp, "./%s", simulator_filename);
            if ( execl (tmp, simulator_filename, (char *) 0) )
                error_exit ("execl error");
        }
	
   /* parent */
   sprintf (child_fifoname, "to-%d", pid);
   mkfifo (child_fifoname, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	
   if ( (fifo_fdout = open (child_fifoname, O_WRONLY)) < 0) 
     error_exit ("load_simulator() open fifoout error");
	
   if ( (fifo_fdin = open (parent_fifoname, O_RDONLY)) < 0) 
     error_exit ("load_simulator() open fifoin error");
}


void 
sig_child (int signo) {
   pid_t   pid;

   while ( (pid = waitpid(-1, NULL, WNOHANG)) > 0);
   if ( (signal(SIGCHLD, sig_child)) == SIG_ERR)
      error_exit("signal error");
}


static gint
comp_ts (FullMsgHeader *a, FullMsgHeader *b) {
   if (a->time_stamp < b->time_stamp) return -1;
      else if (a->time_stamp == b->time_stamp) return 1;
              else return 1;
}


LPInfo *
find_lp_with_min_channel_time (void) {
   int        i, c = 1;
   LPInfo   * lp, *curlp;
   
   /* solo se la coda locale non e' vuota ne viene controllato il channel_time 
	* e confrontato con il channel_time degli altri canali di ingresso */
   if (local_lp->queue) {
     #ifdef DEBUG
      printf ("coda_in local  channel_time=%.2f\n", local_lp->channel_time[0]);
      stampa_coda (local_lp, 1);
     #endif
      curlp = local_lp;
      c = 0;
   } else 
      curlp = (LPInfo *)g_ptr_array_index (channels_in, 0);
	   
   for (i = c; i < channels_in->len-1; i++) {
      lp = (LPInfo *) g_ptr_array_index (channels_in, i);
      if (lp->channel_time[0] <= curlp->channel_time[0]) {
        #ifdef DEBUG
         printf ("%s %.2f <= %s %.2f\n", lp->name,lp->channel_time[0],
                 curlp->name,curlp->channel_time[0]);
        #endif
        curlp = lp; } 
   }

   return curlp;
}

/* dato un nome di LP, restituisce un puntatore alla corrispondente struttura LPInfo */
LPInfo *
find_lp_by_name (char lp_name[]) {
   int      i;
   LPInfo  *lp;

   if (strcmp (lp_name, local_lp->name)==0)
      return local_lp;

   for (i = 0; i < channels_out->len; i++) {
      lp = (LPInfo *)g_ptr_array_index (channels_out, i);
      if ( strcmp(lp->name, lp_name)==0 ) 
         return lp;
   }
   return NULL;
}

/* riceve gli eventuali nuovi eventi generati dal simulatore.
 * Sincronizza anche il clock se non gli è stato inviato niente da processare */
void
receive_replay_from_simulator (gboolean msg_sent) {
   char              end_char, type;
   char              target[15];
   gboolean          end = FALSE;
   LPInfo          * lp;
   FullMsgHeader   * f;
		
   /* passa il controllo al simulatore del PP */
   type = (msg_sent ? END_MSG : SYNC_MSG);
   if ( write (fifo_fdout, &type, sizeof(char)) < 0 )
       error_exit ("simulate() end_char write error:");
   if (!msg_sent)
      if ( write (fifo_fdout, &clock, sizeof(double)) < 0 )
         error_exit ("simulate() clock write error:");

   /* attendo msg da trasmettere (se ho canali di out) */
   while (!end) {
       if ( read (fifo_fdin, &type, sizeof(char)) < 0 )
           error_exit ("simulate read error:");
       switch (type) {
          case TRUE_MSG:
              f = (FullMsgHeader *)malloc (sizeof(FullMsgHeader));
              f->type = FULL_MSG;
              if ( read (fifo_fdin, &(f->time_stamp), sizeof(f->time_stamp)) < 0 )
                 error_exit ("simulate() read error:");
              if ( read (fifo_fdin, &target, sizeof(target)) < 0 )
                 error_exit ("simulate() read error:");
              if ( read (fifo_fdin, f->msg, sizeof(f->msg)) < 0 )
                 error_exit ("simulate() read error:");
             #ifdef DEBUG
              printf ("ricev da sim.  ts=%f  target=%s  msg=%s\n", 
                       f->time_stamp, target, f->msg);
             #endif
				
              lp = find_lp_by_name (target);

              if (!lp) {
                 printf ("Simulate() ERRORE!!! lp con nome `%s` non esiste !!\n",
                          target);
                 exit(1);
              }
              /* se il simulatore ha usato schedule_local_event() */
              if (lp == local_lp) {
                 local_lp->queue = g_list_insert_sorted (local_lp->queue, 
                                                         (gpointer)f,
                                                         (GCompareFunc)comp_ts);
                 local_lp->channel_time[0] = ((FullMsgHeader *)(local_lp->queue->data))->time_stamp;
			  } else {    /* se il simulatore ha usato schedule_extern_event() */
                 new_msg = TRUE;
                 lp->queue_out = g_list_insert_sorted (lp->queue_out, 
                                                      (gpointer)f,
                                                      (GCompareFunc)comp_ts);
			  }
              break;

          /* puo' consentire arresto in casi particolari: il simulatore alla ricezione di un certo msg
		   * potrebbe voler arrestare la simulazione prima del clock finale (non terminata implementazione !) */
          case STOP_MSG:
              stop_received = TRUE;
              end = TRUE;
              break;
			  
          /* il simulatore ha restituito il controllo */
          case END_MSG:
              end = TRUE;
       }
   }
}
		  

int
simulate (void) {
   Msg              msg;
   GList          * first;
   FullMsgHeader  * f;
   NullMsgHeader  * n, *new_null;
   MsgHeaderIPC     msgh_ipc;
   gboolean         is_empty, msg_sent = FALSE;
   int              n_ch = channels_in->len, i;
   LPInfo         * lp, *curlp;
   
  #ifdef DEBUG
   printf ("____SIMUL____\n");
  #endif
   while ((n_ch > 0) && !end_condition_reached) {
       curlp = find_lp_with_min_channel_time (); // chiaro...
	
       /* sono necessari questi controlli in quanto la coda locale è una GList,
		* mentre le code di ingresso sono delle GQueue, di conseguenza le operazioni
		* per estrarre un elemento sono diverse... */   
       if (curlp != local_lp)
          is_empty = g_queue_is_empty (curlp->queue_in);
       else
          is_empty = (curlp->queue == NULL ? TRUE : FALSE);
       if (!is_empty) {
          if (curlp != local_lp) // se il channel_time minimo e' in una coda di ingresso
             f = (FullMsgHeader *)g_queue_pop_head (curlp->queue_in);
          else {   // se il channel_time minimo e' nella coda locale
             first = curlp->queue;
             f = (FullMsgHeader *)first->data;
             curlp->queue = g_list_remove_link (curlp->queue, curlp->queue);
          }
		  // ora f punta al messaggio processabile
		  
          clock = f->time_stamp;
		  
         #ifdef DEBUG
          printf ("Selezionato evento '%.2f %s' da %s\n", f->time_stamp, 
                                             (f->type==FULL_MSG?f->msg:NULL),
                                             curlp->name);
		  printf ("[(clock)]=%.2f\n", clock);
         #endif

          if (clock >= end_clock)
             end_condition_reached = TRUE;
		  
          if (curlp != local_lp) {
             if (!g_queue_is_empty (curlp->queue_in)) {
                curlp->channel_time[0] = 
                   ((FullMsgHeader *)g_queue_peek_head (curlp->queue_in))->time_stamp;
             } else
                curlp->channel_time[0] = f->time_stamp;
          } else {
             if (curlp->queue) {
                curlp->channel_time[0] = ((FullMsgHeader *)(curlp->queue->data))->time_stamp;
             } else
                curlp->channel_time[0] = f->time_stamp;
          }

          if ((f->type == NULL_MSG) && end_condition_reached)  f->type = FULL_MSG;
          if (f->type == FULL_MSG) {
             /* crea un pacchetto con formato MsgHeaderIPC che contiene anche 
			  * il mittente del messaggio */
             memset (&msgh_ipc, 0, sizeof(MsgHeaderIPC));
             msgh_ipc.type = TRUE_MSG;
             msgh_ipc.time_stamp = f->time_stamp;
             strcpy (msgh_ipc.target, curlp->name);
             memmove (msgh_ipc.msg, f->msg, 24);
			 
             if ( write (fifo_fdout, &msgh_ipc.type, sizeof(msgh_ipc.type)) < 0 )
                error_exit ("simulate() msgh_ipc write error:");
             if ( write (fifo_fdout, &msgh_ipc.time_stamp, sizeof(msgh_ipc.time_stamp)) < 0 )
                error_exit ("simulate() msgh_ipc write error:");
             if ( write (fifo_fdout, msgh_ipc.target, sizeof(msgh_ipc.target)) < 0 )
                error_exit ("simulate() msgh_ipc write error:");
             if ( write (fifo_fdout, msgh_ipc.msg, sizeof(msgh_ipc.msg)) < 0 )
                error_exit ("simulate() msgh_ipc write error:");

             free (f);
             if (curlp == local_lp) g_list_free_1 (first);
             msg_sent = TRUE;
             receive_replay_from_simulator (TRUE);
          } else {                       
             /* forward del messaggio null su tutte le code di out */
             new_msg = TRUE;
             for (i = 0; i < channels_out->len; i++) {
                lp = (LPInfo *) g_ptr_array_index (channels_out, i);
                new_null = (NullMsgHeader *)malloc (sizeof(NullMsgHeader));
                new_null->type = NULL_MSG;
				new_null->time_stamp = clock + lp->lookahead;
                lp->queue_out = g_list_insert_sorted (lp->queue_out, 
                                                      (gpointer)new_null,
                                                      (GCompareFunc)comp_ts);
             }
          }
       } else break;   // non ci sono più msg processabili per ora
	   
   }
   if (!msg_sent)
      receive_replay_from_simulator (FALSE);
}


void
main_loop (void) {
   gboolean  flushed = FALSE;

   clock = 0.0;
   end_condition_reached = FALSE;
   stop_received = FALSE;

   /* mando msg null solo se esistono canali di uscita ! */
   if (channels_out->len > 0) 
      send_null_msg_to_all (clock);
	
   while ( (!end_condition_reached || (active_in_channels > 0)) && !stop_received) {
       new_msg = FALSE;
	                                
       if ( (channels_in->len > 1) && (active_in_channels > 0) )  
          recv_msg_from_lp ();

       if (!end_condition_reached)
          simulate ();

       /* se sono stati generati nuovi msg potenzialmente spedibili, simulate()
		* setta new_msg a TRUE, al contrario send_msg_to_lp() non viene chiamata.
		* Solo quando viene raggiunta la condiz. di arresto, viene chiamata 
		* flush_all_out_queue() che spedisce tutti i msg con time_stamp <= end_clock */
       if ( (channels_out->len > 0) ) 
          if (end_condition_reached) {
             if (!flushed) {
                flush_all_out_queue ();
                flushed = TRUE;
             }
          }
          else
             if (new_msg) send_msg_to_lp ();
   }
}


int 
main(int argc, char *argv[]) 
{
   int res;
   char  close_char = CLOSE_MSG;
	
   if (argc != 2) {
      printf ("usage:  lp <nome_file_config>\n");
      exit(1);
   }
	
   if ( (signal (SIGCHLD, sig_child)) == SIG_ERR)
      error_exit("signal error");
   
   printf ("     ******************************************\n");
   printf ("  ***** DDSim - DynamicDistribuitedSimulator *****\n");
   printf ("*******           2003 Marco Mussini         *******\n");
   printf ("  *****          mussini@cs.unibo.it         *****\n");
   printf ("     ******************************************\n");

   /* legge il file di configurazione */
   res = parse_config_file (argv[1]);
	
   res = connect_to_all_my_neighbours ();

   /* carica il simulatore e crea fifo di comunicazione */   
   load_simulator ();

   /* ciclo principale di Chandy-Misra */
   main_loop ();
	
   /* chiude la fifo con il simulatore */
   if ( write (fifo_fdout, &close_char, sizeof(char)) < 0 )
      error_exit ("main_loop() write error:");
   if ( read (fifo_fdin, &close_char, sizeof(char)) < 0 )
      error_exit ("main_loop() read error:");
   
   close (fifo_fdin);
   close (fifo_fdout);
	
   if (unlink (child_fifoname) < 0) 
      print_error ("UNLINK error:");
   if (unlink (parent_fifoname) < 0) 
      print_error ("UNLINK to-p:");	
}
