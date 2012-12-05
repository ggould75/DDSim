#ifndef  __IPC_LPSIM_H_
#define  __IPC_LPSIM_H_

#include "msg_header_ipc.h"


void     init_communication (char this_lp_name[]);

void     close_communication (void);

void     schedule_extern_event (double time_stamp, char dest[], char msg[]);

void     schedule_local_event (double time_stamp, char msg[]);

GQueue * receive_event (void);

void     sim_P (void);

void     sim_V (void);


/* il simulatore non deve dichiarare una variabile per rappresentare il tempo simulato
 * e' gia' dichiarata qui */
double   clock;
char     local_lp_name[15];  // il mio nome è...

#endif
