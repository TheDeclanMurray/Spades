CC := clang
CFLAGS := -g

all: server client

clean:
	rm -rf server client

server: server.c message.h message.c socket.h gamestate.c gamestate.h send_messages.h send_messages.c
	$(CC) $(CFLAGS) -o server server.c message.c gamestate.c send_messages.c -lpthread

client: client.c message.h message.c
	$(CC) $(CFLAGS) -o client client.c message.c

