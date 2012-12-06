#include "globals.h"


/**
 * Prints an error and exits
 */
void 
error_exit (const char *str) {
    char strmess[1024];
	
    sprintf ((char *)strmess, "%s, Err:%d \"%s\"\n", str,
                                                     errno, 
                                                     strerror(errno));
    fprintf (stderr, "%s", strmess);
    fflush (stderr);
    exit (1);
}

/**
 * Prints an error
 */
void 
print_error (const char *str) {
    char strmess[1024];
	
    sprintf ((char *)strmess, "%s, Err:%d \"%s\"\n", str,
                                                     errno, 
                                                     strerror(errno));
    fprintf (stderr, "%s", strmess);
    fflush (stderr);
}

/**
 * Debug only.
 * Print the local queue of an LP
 */
void
stampa_coda (LPInfo *lp, int what) {
    GList *tmp = (what == 0 ? lp->queue_out : lp->queue);
	Msg   *msg;
	
	while (tmp != NULL) {
        msg = (Msg *)(tmp->data);
	    if (msg->full.type == 'F')
	       printf("'%.2f %s' ", ((FullMsgHeader *)(tmp->data))->time_stamp, ((FullMsgHeader *)(tmp->data))->msg);
	    else printf("'%.2fN' ", ((NullMsgHeader *)(tmp->data))->time_stamp);
	    tmp = tmp->next;
	}
    printf("\n");
}

/**
 * Debug only.
 * Print the queue_in of an LP
 */
void
stampa_coda_in (LPInfo *lp) {
    GList *tmp;
	Msg   *msg;
    int    a;
	
    tmp = lp->queue_in->head;
    for (a=0; a<lp->queue_in->length; a++) {
        msg = (Msg *)tmp->data;
        if (msg->full.type == 'F')
           printf("'%.2f %s' ", ((FullMsgHeader *)(tmp->data))->time_stamp, 
                            ((FullMsgHeader *)(tmp->data))->msg);
		else printf("'%.2fN' ", ((NullMsgHeader *)(tmp->data))->time_stamp);
	    tmp = tmp->next;
    }
    printf("\n");
}
