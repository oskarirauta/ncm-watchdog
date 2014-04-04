CC=gcc
CFLAGS=-I. -DRESOLVCONF_LOCATION="\"/tmp/resolv.conf.auto\""
LDFLAGS=

LIBS=-lubus -lubox -lblobmsg_json
OBJS=objs/ping.o objs/hostlist.o objs/util.o objs/main.o

all: ncm-watchdog

objs:
	mkdir objs

objs/ping.o: objs ping.c
	$(CC) -c ping.c -o objs/ping.o $(CFLAGS)

objs/hostlist.o: objs hostlist.c
	$(CC) -c hostlist.c -o objs/hostlist.o $(CFLAGS)

objs/util.o: objs util.c
	$(CC) -c util.c -o objs/util.o $(CFLAGS)

objs/main.o: objs main.c
	$(CC) -c main.c -o objs/main.o $(CFLAGS)

ncm-watchdog: $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $@ $(OBJS)

clean:
	rm -r objs/*.o a.out

distclean:
	rm -rf objs a.out
