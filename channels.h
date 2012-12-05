#ifndef  __CHANNELS_H_
#define __CHANNELS_H_

#include "globals.h"


int parse_config_file (char *filename);

int connect_to_all_my_neighbours (void);

void send_null_msg_to (LPInfo *lp, double time_stamp);

void send_null_msg_to_all (double base);

int recv_msg_from_lp (void);

int send_msg_to_lp (void);

void flush_all_out_queue (void);

#endif
