all: server client

$(SEL_SRV):$server.c
	gcc -o server -lnsl server.c

$(CLT):	client.c
	gcc -o client -lnsl client.c

clean:
	rm -f *.o *~
	rm -f server client


