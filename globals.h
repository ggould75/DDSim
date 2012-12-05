#ifndef  __GLOBALS_H_
#define  __GLOBALS_H_

#include <glib.h>
#include <errno.h>
#include <stdio.h>

/* decommentare la riga seguente per vedere nei dettagli 
 * il comportamento dell'algoritmo */
//#define  DEBUG           1

#define  MAX_NAME_LEN   15
#define  NULL_MSG       'N'
#define  FULL_MSG       'F'


typedef struct _LPInfo             LPInfo;
typedef struct _full_msg_header    FullMsgHeader;
typedef struct _null_msg_header    NullMsgHeader;
typedef union   msg                Msg;

/* ogni LP puo' avere piu' canali identificati in questo modo */
typedef enum {
    IN,
    OUT,
    BOTH,
    LOCAL
} ChannelType;

/* i dati di un LP vicino sono memorizzati in questa struttura */
struct _LPInfo {
    char                 name[MAX_NAME_LEN];
    int                  fd[2]; // fd[0] usato per IN, fd[1] per OUT
    char                 ip_address[16];
    unsigned short int   port;  // porta remota
    GQueue             * queue_in;  // punta a null se il canale è di solo OUT
    GList              * queue_out; // punta a null se il canale è di solo IN
    GList              * queue;  // coda usata solo da lp locale per schedulare eventi locali
    double               channel_time[2]; // usato solo 0
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

/* i msg spediti in rete dall'LP, usano come formato FullMsgHeader o NullMsgHeader */
union msg {
    FullMsgHeader      full;
    NullMsgHeader      null;
};



/* array di puntatori a strutture LPInfo. 
 * channels_in ha un puntatore per ogni canale di ingresso
 * channels_out un puntatore per ogni canale di uscita
 * i canali di tipo BOTH hanno un puntatore sia in channels_in che channels_out */
GPtrArray * channels_in, *channels_out;

FILE      * config_file;
char        config_filename[25];
char        simulator_filename[25];
int         fifo_fdin, fifo_fdout; // fifo per comunicaz. con simulatore

/* this lp data */
LPInfo    * local_lp;
double      clock;
double      end_clock;
gboolean    stop_received;
int         active_in_channels; // numero di canali di ingresso ancora attivi
                                // se raggiunge 0, l'LP non legge più i messaggi
                                // dai canali di ingresso

void error_exit  (const char *str);
void stampa_coda (LPInfo *lp, int what);
void stampa_coda_in (LPInfo *lp);

#endif
