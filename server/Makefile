CC=$(CROSS_COMPILE)gcc
CFLAGS += -Wall -Werror

all : aesdsocket.o daemonize.o
	$(CC) aesdsocket.o daemonize.o -o aesdsocket

aesdsocket.o: aesdsocket.c
	$(CC) -c aesdsocket.c -o aesdsocket.o $(CFLAGS)
	
daemonize.o: daemonize.c
	$(CC) -c daemonize.c -o daemonize.o $(CFLAGS)	

clean :
	rm -f aesdsocket \
	rm -f *.o
	
	
  