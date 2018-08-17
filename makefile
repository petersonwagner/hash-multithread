CC = gcc
CFLAGS = -O3 -D_REENTRANT

all: main put_client get_client

main: main.c hash.c avltree.c crypto.c
	$(CC) -O3 -D_REENTRANT main.c -o main -g hash.c avltree.c crypto.c -lpthread -lcrypto

put_client: put_client.c casanova.h
	$(CC) $(CFLAGS) -o put_client put_client.c

get_client: get_client.c casanova.h
	$(CC) $(CFLAGS) -o get_client get_client.c

clean: 
	rm -f *.o *.*~ *~ put_client get_client casanova main
	rm -f *.o *.*~ *~ telefones server_put_socket server_get_socket
       
