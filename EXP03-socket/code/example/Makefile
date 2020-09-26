all: http-client http-server

http-client: http-client.c
	gcc -Wall -g http-client.c -o http-client

http-server: http-server.c
	gcc -Wall -g http-server.c -o http-server

clean:
	@rm -f http-client http-server
