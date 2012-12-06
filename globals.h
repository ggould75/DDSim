#ifndef  __GLOBALS_H_
#define  __GLOBALS_H_

#include <glib.h>
#include <errno.h>
#include <stdio.h>


/**
 * Uncomment to see how the algorithm works
 */
//#define  DEBUG           1

#define  MAX_NAME_LEN   15
#define  NULL_MSG       'N'
#define  FULL_MSG       'F'


typedef struct _LPInfo             LPInfo;
typedef struct _full_msg_header    FullMsgHeader;
typedef struct _null_msg_header    NullMsgHeader;
typedef union   msg                Msg;

/**
 * Possible channels for an LP
 */
typedef enum {
    IN,
    OUT,
    BOTH,
    LOCAL
} ChannelType;

/**
 * LP data
 */
struct _LPInfo {
    char                 name[MAX_NAME_LEN];
    int                  fd[2]; // fd[0] used for IN, fd[1] for OUT
    char                 ip_address[16];
    unsigned short int   port;  // remote port
    GQueue             * queue_in;  // null if it is an out channel
    GList              * queue_out; // null if it is an in channel
    GList              * queue;  // This queue is only for the local LP, in order to schedule local events
    double               channel_time[2]; // used only 0
    double               lookahead;
    ChannelType          type;
};

struct _full_msg_header {
    char      type;
    double    time_stamp;
    char      msg[24];
};

struct _null_msg_header {
    char      type;
    double    time_stamp;
};


/**
 * The two kind of messages sent through the network by an LP
 */
union msg {
    FullMsgHeader      full;
    NullMsgHeader      null;
};



/**
 * Arrays of LPInfo structures.
 * channels_in has a pointer for each input channel
 * channels_out has a pointer for each output channel
 * The channels of type BOTH have a pointer both in channels_in and channels_out
 */
GPtrArray * channels_in, *channels_out;

FILE      * config_file;
char        config_filename[25];
char        simulator_filename[25];
int         fifo_fdin, fifo_fdout; // fifo to communicate with the simulator

/* this lp data */
LPInfo    * local_lp;
double      clock;
double      end_clock;
gboolean    stop_received;
int         active_in_channels; // number of input channels still active
                                // if 0 is reached, the LP will no longer read the messages from the input channels

void error_exit  (const char *str);
void stampa_coda (LPInfo *lp, int what);
void stampa_coda_in (LPInfo *lp);

#endif
