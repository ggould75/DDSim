#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include "channels.h"


fd_set   rset, allset, allset_in;
int      maxfd = 0, maxfd_in = 0;
int      n_both;
int      n_in;
int      n_out;


int 
readn (int fd, char *ptr, int n) {
   size_t nleft;
   ssize_t nread;

   nleft = n;
   while (nleft > 0) {
      if ( (nread = read(fd, ptr, nleft)) < 0) {
         if (errno == EINTR) nread = 0;
            else {
               perror("read(...) in readn failed");
               return -1;
            }
      } else if (nread == 0) break; // EOF
      nleft -= nread;
      ptr += nread;	
   }
   return (n-nleft);
}


ssize_t 
writen (int fd, const void *buf, size_t n) {
   size_t nleft;
   ssize_t nwritten;
   char *ptr;

   ptr = (char *)buf;
   nleft = n;
   while (nleft > 0) {
      if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
         if (errno == EINTR) nwritten = 0;
            else {
               perror("write(...) in writen failed");
               return -1;
            }
      }
      nleft -= nwritten;
      ptr += nwritten;
   }
   return n;
}


void
send_null_msg_to (LPInfo *lp, double time_stamp) {
   int              res;
   NullMsgHeader    null_msg;

   memset (&null_msg, 0, sizeof(NullMsgHeader));
   null_msg.type = NULL_MSG;   
   null_msg.time_stamp = time_stamp;	  
   res = writen (lp->fd[1], &null_msg, sizeof(NullMsgHeader));
   if (res < 0) print_error ("send_null_msg_to() writen error:");
  #ifdef DEBUG
   printf ("Sent 'null, %.2f' to %s\n", null_msg.time_stamp, lp->name);
  #endif
}


void
send_null_msg_to_all (double base) {
   int              i;
   LPInfo         * lp;

   for (i = 0; i < channels_out->len; i++) {
      lp = (LPInfo *)g_ptr_array_index (channels_out, i);
      send_null_msg_to (lp, base + lp->lookahead);
   }
}


int
send_msg_to_lp (void) {
   int              i, c, res;
   double           curr_ts, last_ts_sent;
   static double    last_clock = -1.0;
   LPInfo         * lp, *lp_in;
   FullMsgHeader  * f;
   gboolean         cond_ok, ok_tosend;
   GList          * first, *next_msg;
	
  #ifdef DEBUG
   printf ("______SEND______\n");
  #endif
   
   for (i = 0; i < channels_out->len; i++) {
      lp = (LPInfo *) g_ptr_array_index (channels_out, i);
     #ifdef DEBUG	
      printf ("coda_out di %s: ", lp->name);
	  stampa_coda (lp, 0);
     #endif	  
	  
      while (lp->queue_out) {
         first = lp->queue_out;
         next_msg = first->next;
         f = (FullMsgHeader *)first->data;
         curr_ts = f->time_stamp;
         ok_tosend = TRUE;
         cond_ok = TRUE;
		 
         if (curr_ts <= clock + lp->lookahead) {
             // useless!?
         } else break;

         switch (f->type) {
            case FULL_MSG:
               res = writen (lp->fd[1], f, sizeof(FullMsgHeader));
               if (res < 0) print_error ("send_msg_to_lp() writen1 error:");
              #ifdef DEBUG
               printf ("Sent '%s, %.2f' to %s\n", f->msg, f->time_stamp, lp->name);
              #endif
			   break;
			   
            case NULL_MSG:
               if (next_msg != NULL) {
                  if (((FullMsgHeader *)next_msg->data)->type == NULL_MSG)  ok_tosend = FALSE;
               } else ok_tosend = FALSE;
               if ((clock == last_clock) || (curr_ts == clock+lp->lookahead))  ok_tosend = FALSE;
               if (ok_tosend) {
                  res = writen (lp->fd[1], (NullMsgHeader *)f, sizeof(NullMsgHeader));
                  if (res < 0) print_error ("send_msg_to_lp() writen2 error:");
                 #ifdef DEBUG
                  printf ("Sent 'null, %.2f' to %s\n", f->time_stamp, lp->name); 
                 #endif
               }
			    #ifdef DEBUG
                 else printf ("'null %.2f' to %s scartato\n", f->time_stamp, lp->name);
                #endif
         }

         last_ts_sent = curr_ts;
         lp->queue_out = g_list_remove_link (lp->queue_out, lp->queue_out);
         free (f);
         g_list_free_1 (first);
      }
   }
   
   // Send a null msg on all the output channels using the lower bound of the future sent messsages
   // as ts (= clock + out channel lookahead)
   if (clock != last_clock) {
     #ifdef DEBUG
	  printf("start to send null msg\n");
     #endif
      send_null_msg_to_all (clock);
      last_clock = clock;
   }
}


void
flush_all_out_queue (void) {
   int              i, c, res;
   double           curr_ts, last_ts_sent;
   LPInfo         * lp, *lp_in;
   FullMsgHeader  * f;
   gboolean         cont, msg_sent;
   GList          * first;

  #ifdef DEBUG
   printf ("________FLUSH________\n");
  #endif
   for (i = 0; i < channels_out->len; i++) {
      lp = (LPInfo *) g_ptr_array_index (channels_out, i);
      cont = TRUE;
	  
     #ifdef DEBUG
      printf ("coda_out di %s: ", lp->name);
	  stampa_coda (lp, 0);
     #endif
      while (lp->queue_out && cont) {
         first = lp->queue_out;
         f = (FullMsgHeader *)first->data;
         curr_ts = f->time_stamp;

		 if (curr_ts <= end_clock) {
            switch (f->type) {
               case FULL_MSG:
                  res = writen (lp->fd[1], f, sizeof(FullMsgHeader));
                  if (res < 0) print_error ("flush_all_out_queue() writen1 error:");
                 #ifdef DEBUG
                  printf ("Sent '%s, %.2f' to %s\n", f->msg, f->time_stamp, lp->name);
                 #endif
                  msg_sent = TRUE;
                  break;
			   
               case NULL_MSG:
                  res = writen (lp->fd[1], (NullMsgHeader *)f, sizeof(NullMsgHeader));
                  if (res < 0) print_error ("flush_all_out_queue() writen1 error:");
                 #ifdef DEBUG
                  printf ("Sent 'null, %.2f' to %s\n", f->time_stamp, lp->name);
                 #endif
            }

            last_ts_sent = curr_ts;
            lp->queue_out = g_list_remove_link (lp->queue_out, lp->queue_out);
            free (f);
            g_list_free_1 (first);
         } else
            cont = FALSE;
      }
   }
 
  #ifdef DEBUG
   printf("start to send null msg\n");
  #endif
   for (i = 0; i < channels_out->len; i++) {
      lp = (LPInfo *)g_ptr_array_index (channels_out, i);
      send_null_msg_to (lp, clock + lp->lookahead);
      shutdown (lp->fd[1], SHUT_RDWR);
   }
}


// The first time it is called forward the ownership only after receiving at least one msg
// on each input channel
int 
recv_msg_from_lp (void) {
   int                i, res;
   char               tmp[40], type;
   gboolean           at_least_one_msg;
   static gboolean    first = TRUE;
   gpointer           data;
   LPInfo           * lp;
   Msg                msg;
   FullMsgHeader    * f;
   NullMsgHeader    * n;
   
  #ifdef DEBUG
   printf ("__RECV__\n");
  #endif
   do {
      at_least_one_msg = TRUE;
      rset = allset_in;
      res = select (maxfd_in+1, &rset, NULL, NULL, NULL);
      if (res < 0) error_exit ("select failed:");
      if (res != 0) {
         for (i = 0; i < channels_in->len-1; i++) {
            lp = (LPInfo *) g_ptr_array_index (channels_in, i);
			
            if (FD_ISSET (lp->fd[0], &rset)) {
               res = readn (lp->fd[0], (char *)&msg, sizeof(NullMsgHeader));			
               if (res < 0) 
                  print_error ("recv_msg_from_lp() readn error:");
			   
               if (res == 0) {
                  FD_CLR (lp->fd[0], &allset_in);
                  active_in_channels--;
               } else {			   
                  switch (msg.full.type) {
                     case FULL_MSG:
                        res = readn (lp->fd[0], (char *)&msg.full.msg, sizeof(msg.full.msg));
                        if (res < 0) 
                           print_error ("recv_msg_from_lp() readn2 error:");
                        f = (FullMsgHeader *)malloc (sizeof(FullMsgHeader));
                        f->type = msg.full.type;
                        f->time_stamp = msg.full.time_stamp;
                        memmove (f->msg, msg.full.msg, 24);
                        data = (gpointer)f;
                        break;
					 
                     case NULL_MSG:
                        n = (NullMsgHeader *)malloc (sizeof(NullMsgHeader));
                        n->type = msg.null.type;
                        n->time_stamp = msg.full.time_stamp;
                        data = (gpointer)n;
                        break;
				   	 
                     default:
                        fprintf (stderr, "Bad type of msg received !!!");
                        exit(1);
                  }
			   
                  if (g_queue_is_empty (lp->queue_in))
                     lp->channel_time[0] = msg.full.time_stamp;
			   
                  g_queue_push_tail (lp->queue_in, data);
                 #ifdef DEBUG
                  printf ("msg received from lp '%s': ts=%.2f '%s'\n",
                           lp->name, 
                           msg.full.time_stamp, 
                           (msg.full.type == FULL_MSG ? msg.full.msg:NULL));
                  printf ("coda_in di %s:\n", lp->name);
                  stampa_coda_in (lp);			   
                 #endif
               }
			}
            if (first) at_least_one_msg = at_least_one_msg && 
                                          (lp->channel_time[0] > 0.0);
         } // end for
      } // end if res!=0
   } while (!at_least_one_msg && first && (active_in_channels > 0));
   
   if (first) first = FALSE;
}


static void
create_channel (LPInfo *lp, 
                ChannelType type, 
                unsigned short int local_port) {
    struct sockaddr_in Local;
    int fd, OptVal = 1, res;
	
    fd = socket (AF_INET, SOCK_STREAM, 0);
    if (fd < 0) error_exit ("socket() failed:");
    res = setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, 
                     (char *)&OptVal, sizeof(OptVal));
    if (res < 0) error_exit ("setsockopt() failed\n");
	
    if (type == IN) {
        memset (&Local, 0, sizeof(Local));
        Local.sin_family = AF_INET;
        Local.sin_addr.s_addr = htonl (INADDR_ANY);
        Local.sin_port = htons (local_port);
        res = bind (fd, (struct sockaddr *) &Local, sizeof(Local));
        if (res < 0) error_exit ("bind() failed:");
		lp->fd[0] = fd;
        res = listen (fd, 50);
        if (res < 0) error_exit("listen() failed:");
    } else
        lp->fd[1] = fd;
}


// Read config.
// Notice that there are no correctness checks on this file!
int
parse_config_file (char *filename) {		
	int      local_port_number;
	int      i;
	float    tmp;
    char     my_lp_name[MAX_NAME_LEN];
	LPInfo * lpinfo;
	
	strcpy (config_filename, filename);
    if ( (config_file = fopen (config_filename, "r")) == NULL ) 
        error_exit ("fopen error:");
    fscanf (config_file, "%s\n%s\n%f\n", my_lp_name, simulator_filename, &tmp);
    fscanf (config_file, "%d\n%d\n%d\n", &n_both, &n_in, &n_out);
    end_clock = (double)tmp;
    printf ("LP name: %s\nSimulator file name: %s\nboth/in/out channels: %d, %d, %d\nsimulation stop at clock: %f\n", 
                my_lp_name, simulator_filename, n_both, n_in, n_out, end_clock);
    printf ("------------------------------------\nChannel list:\n");
    channels_in = g_ptr_array_new ();
    channels_out = g_ptr_array_new ();
	
    FD_ZERO (&allset);
    
    for (i = 0; i < n_both; i++) {
        lpinfo = (LPInfo *) malloc (sizeof (LPInfo));
		fscanf (config_file, "%d\n", &local_port_number);
        fscanf (config_file, "%s\n", lpinfo->ip_address);
        fscanf (config_file, "%d\n", &lpinfo->port);
        fscanf (config_file, "%s\n", lpinfo->name);
        fscanf (config_file, "%f\n", &tmp);
        lpinfo->lookahead = (double)tmp;
        lpinfo->channel_time[0] = lpinfo->channel_time[1] = 0.0;
        lpinfo->type = BOTH;
        lpinfo->queue_in = g_queue_new ();
        lpinfo->queue_out = NULL;
        g_ptr_array_add (channels_in, (gpointer)lpinfo);
        g_ptr_array_add (channels_out, (gpointer)lpinfo);
        create_channel (lpinfo, IN, (unsigned short int)local_port_number);
        create_channel (lpinfo, OUT, 0);
        if (lpinfo->fd[0] > maxfd) maxfd = lpinfo->fd[0];
		FD_SET (lpinfo->fd[0], &allset);
        printf ("Type: BOTH, Local_port:%d\n      neighbour name/IP/port: %s/%s:%d, Lookahead=%.2f\n", 
                    local_port_number, 
                    lpinfo->name, 
                    lpinfo->ip_address, 
                    lpinfo->port,
                    lpinfo->lookahead);
    }
    for (i = 0; i < n_in; i++) {
        lpinfo = (LPInfo *) malloc (sizeof (LPInfo));
		fscanf (config_file, "%d\n", &local_port_number);
        fscanf (config_file, "%s\n", lpinfo->name);
        fscanf (config_file, "%f\n", &tmp);
        lpinfo->lookahead = (double)tmp;
        lpinfo->channel_time[0] = lpinfo->channel_time[1] = 0.0;
        lpinfo->type = IN;
        lpinfo->queue_in = g_queue_new ();
        lpinfo->queue_out = NULL; 
        g_ptr_array_add (channels_in, (gpointer)lpinfo);
        create_channel (lpinfo, IN, (unsigned short int)local_port_number);
        if (lpinfo->fd[0] > maxfd) maxfd = lpinfo->fd[0];
        FD_SET (lpinfo->fd[0], &allset);
        printf ("Type: IN, Local_port:%d\n      neighbour name:%s, Lookahead=%.2f\n", 
                    local_port_number, 
                    lpinfo->name,
                    lpinfo->lookahead);
    }
    for (i = 0; i < n_out; i++) {
        lpinfo = (LPInfo *) malloc (sizeof (LPInfo));
        fscanf (config_file, "%s\n", lpinfo->ip_address);
        fscanf (config_file, "%d\n", &lpinfo->port);
        fscanf (config_file, "%s\n", lpinfo->name);
        fscanf (config_file, "%f\n", &tmp);
        lpinfo->lookahead = (double)tmp;
        lpinfo->channel_time[0] = lpinfo->channel_time[1] = 0.0;
        lpinfo->type = OUT;
        lpinfo->queue_in = NULL;
        lpinfo->queue_out = NULL;
        g_ptr_array_add (channels_out, (gpointer)lpinfo);
        create_channel (lpinfo, OUT, 0);
        printf ("Type: OUT,  Lookahead=%.3f\n      neighbour name/IP/port: %s/%s:%d\n", 
                    lpinfo->lookahead,
                    lpinfo->name, 
                    lpinfo->ip_address, 
                    lpinfo->port);
    }

    // Always creates an input channel to schedule local events
    lpinfo = (LPInfo *)malloc (sizeof (LPInfo));
    local_lp = lpinfo;
    strcpy (lpinfo->name, my_lp_name);
    lpinfo->channel_time[0] = 0.0;
    lpinfo->type = LOCAL;
    lpinfo->queue = NULL;
    g_ptr_array_add (channels_in, (gpointer)lpinfo);
	
	active_in_channels = n_in = channels_in->len-1;
	n_out = channels_out->len;
    printf ("------------------------------------\n");	
	printf ("Waiting for connections...");
	fflush (stdout);
}

// it is unlocked only when all its neighbors are connected
int
connect_to_all_my_neighbours (void) {
    struct sockaddr_in   Client, Server;
    int                  fd, res, connfd, len, i;
    int                  all_connected;
    struct               timeval timeout;
	LPInfo               *lp;
	
    FD_ZERO (&allset_in);
    all_connected = (n_in == 0) && (n_out == 0);
	
    while (!all_connected) {
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        
        if (n_in > 0) {
            rset = allset;
            res = select (maxfd+1, &rset, NULL, NULL, &timeout);
            if (res < 0) error_exit ("select failed\n");
			if (res != 0) {
              for (i = 0; i < channels_in->len-1; i++) {
                  lp = (LPInfo *) g_ptr_array_index (channels_in, i);
                  fd = lp->fd[0];
                  if (FD_ISSET (fd, &rset)) {
                      printf("\nChannel IN established with %s", lp->name);
                      memset(&Client, 0, sizeof(Client));
                      len = sizeof(struct sockaddr_in);
                      connfd = accept(fd, (struct sockaddr *) &Client, &len);
                      if (connfd < 0) error_exit ("accept() error:");
                      if (lp->type == IN) {
                          sprintf (lp->ip_address, "%s", inet_ntoa (Client.sin_addr));
                          lp->port = ntohs (Client.sin_port);
                      }
                      lp->fd[0] = connfd;
                      FD_CLR (fd, &allset);
                      FD_SET (connfd, &allset_in);
                      if (connfd > maxfd_in) maxfd_in = connfd;
                      if (--n_in == 0) break;
                  }
              }
            }
        }
		
        if (n_out > 0) {
            if (n_in == 0) 
                do res = select (0, NULL, NULL, NULL, &timeout);
                while( (res < 0 ) && (errno == EINTR) );
            for (i = 0; i < channels_out->len; i++) {
                lp = (LPInfo *) g_ptr_array_index (channels_out, i);
                fd = lp->fd[1];
                memset (&Server, 0, sizeof(Server));
                Server.sin_family = AF_INET;
                Server.sin_addr.s_addr = inet_addr (lp->ip_address);
                Server.sin_port = htons (lp->port);
                res = connect (fd, (struct sockaddr *) &Server, sizeof(Server));
                if (res < 0) continue;
                printf("\nChannel OUT established with %s", lp->name);
                if (--n_out == 0) break;
            }
        }
		
        all_connected = (n_in == 0) && (n_out == 0);
        printf ("..");
	    fflush (stdout);
	}
    printf ("\nALL CONNECTED!\n");
	close (config_file);
}
