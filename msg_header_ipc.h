#ifndef  __MSG_HEADER_IPC_H_
#define  __MSG_HEADER_IPC_H_

#include <glib.h>

#define    TRUE_MSG    '1'
#define    END_MSG     '0'
#define    SYNC_MSG    'Y'
#define    STOP_MSG    'S'
#define    CLOSE_MSG   'C'


typedef struct _msg_header_ipc    MsgHeaderIPC;


/* Header per lo scambio di msg tramite fifo tra ogni simulatore e il suo lp
 * @type: non indica il tipo di evento, ma è usato internamente per lo scambio di msg 
 *        fra lp e simulatore
 * @target: indica LP destinazione quando il simulatore invia msg al suo lp, oppure mittente
 * @msg: puo' trasportare qualsiasi stringa semplice. In base alle esigenze del simulatore e'
 *       possibile incapsulare una sotto-header in questo msg di 24 byte (vedere es. simulatore)
 */
struct _msg_header_ipc {
    char      type;
    double    time_stamp;
    char      target[15];
    char      msg[24];
};

#endif
