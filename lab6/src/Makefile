CC = gcc
CFLAGS = -Wall -Wextra -pthread
LDFLAGS = -L.
LDLIBS = -lcommon -lpthread

all: server client

server: server.o libcommon.a
	$(CC) $(CFLAGS) -o $@ server.o $(LDFLAGS) $(LDLIBS)

client: client.o libcommon.a
	$(CC) $(CFLAGS) -o $@ client.o $(LDFLAGS) $(LDLIBS)

libcommon.a: common.o
	ar rcs $@ common.o

server.o: server.c common.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c common.h
	$(CC) $(CFLAGS) -c client.c

common.o: common.c common.h
	$(CC) $(CFLAGS) -c common.c

clean:
	rm -f *.o server client libcommon.a