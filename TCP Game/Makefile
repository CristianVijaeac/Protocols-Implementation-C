all: client server

client: client64.o link_emulator/lib.o
	gcc -g client64.o link_emulator/lib.o -o client

server: server.o link_emulator/lib.o
	gcc -g server.o link_emulator/lib.o -o server

.c.o: 
	gcc -Wall -g -c $? 

clean:
	rm -f server.o server
