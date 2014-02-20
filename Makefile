all: client

client: fill_headers.o client_get.o client_put.o
	gcc -o client client.c fill_headers.o client_get.o client_put.o

fill_headers.o: fill_headers.c
	gcc -c fill_headers.c

client_get.o: client_get.c
	gcc -c client_get.c

client_put.o: client_put.c
	gcc -c client_put.c

clean:
	rm -f client *.o
