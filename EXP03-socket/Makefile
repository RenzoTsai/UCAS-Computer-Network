all: http-client http-server

echo-client: http-client.c
	gcc -Wall -g http-client.c -o http-client

echo-server: http-server.c
	gcc -Wall -g http-server.c -o http-server

clean:
	@rm -f http-client http-server
