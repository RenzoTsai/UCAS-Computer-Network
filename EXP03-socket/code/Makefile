all: echo-client echo-server

echo-client: echo-client.c
	gcc -Wall -g echo-client.c -o echo-client

echo-server: echo-server.c
	gcc -Wall -g echo-server.c -o echo-server

clean:
	@rm -f echo-client echo-server
