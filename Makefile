SOURCES = lp.c globals.c channels.c simulator_Milano.c simulator_Roma.c simulator_Palermo.c sim_1.c sim_2.c sim_3.c sim_4.c sim_5.c ipc_lp_sim.c
OBJS    = ${SOURCES:.c=.o}
CC = gcc
CFLAGS = `pkg-config --cflags gtk+-2.0`
LDADD = `pkg-config --libs gtk+-2.0`
GLIB_CFLAGS = `pkg-config --cflags glib-2.0`
GLIB_LDADD = `pkg-config --libs glib-2.0`


all: ${OBJS}
	$(CC) -o lp lp.o globals.o channels.o ${GLIB_LDADD}
	$(CC) -o simulator_Milano simulator_Milano.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o simulator_Roma simulator_Roma.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o simulator_Palermo simulator_Palermo.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o sim1 sim_1.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o sim2 sim_2.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o sim3 sim_3.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o sim4 sim_4.o ipc_lp_sim.o ${GLIB_LDADD}
	$(CC) -o sim5 sim_5.o ipc_lp_sim.o ${GLIB_LDADD}

lp.o: lp.c channels.h msg_header_ipc.h
	$(CC) $(GLIB_CFLAGS) -c lp.c

globals.o: globals.c globals.h
	$(CC) $(GLIB_CFLAGS) -c globals.c

channels.o: channels.c channels.h
	$(CC) $(GLIB_CFLAGS) -c channels.c

ipc_lp_sim.o: ipc_lp_sim.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c ipc_lp_sim.c



simulator_Milano.o: simulator_Milano.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c simulator_Milano.c

simulator_Roma.o: simulator_Roma.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c simulator_Roma.c

simulator_Palermo.o: simulator_Palermo.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c simulator_Palermo.c



sim_1.o: sim_1.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c sim_1.c

sim_2.o: sim_2.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c sim_2.c

sim_3.o: sim_3.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c sim_3.c

sim_4.o: sim_4.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c sim_4.c

sim_5.o: sim_5.c ipc_lp_sim.h
	$(CC) $(GLIB_CFLAGS) -c sim_5.c


cleanall:
	rm lp *.o
