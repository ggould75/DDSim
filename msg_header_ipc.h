#ifndef  __MSG_HEADER_IPC_H_
#define  __MSG_HEADER_IPC_H_

#include <glib.h>

#define    TRUE_MSG    '1'
#define    END_MSG     '0'
#define    SYNC_MSG    'Y'
#define    STOP_MSG    'S'
#define    CLOSE_MSG   'C'


typedef struct _msg_header_ipc    MsgHeaderIPC;


/**
 * Header to exchange messages with the fifo between each simulator and its LP.
 * @type: does not indicate the type of the event, but is for internal use to enable the communication
 * @target: represents the destination LP (if the simulator is sending to its LP), otherwise the sender
 * @msg: represents any kind of message. The simulator will be able also to encapsulate a sub-header
 *       inside of this 24 bytes message
 */
struct _msg_header_ipc {
    char      type;
    double    time_stamp;
    char      target[15];
    char      msg[24];
};

#endif
