#include "globals.h"


/* print error msg and exit */
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

/* stampa solo un msg di errore */
void 
print_error (const char *str) {
    char strmess[1024];
	
    sprintf ((char *)strmess, "%s, Err:%d \"%s\"\n", str,
                                                     errno, 
                                                     strerror(errno));
    fprintf (stderr, "%s", strmess);
    fflush (stderr);
}

/* usata solo per debug: stampa coda locale o coda_out di lp specificato */
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

/* usata solo per debug: stampa coda_in di lp specificato */
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
