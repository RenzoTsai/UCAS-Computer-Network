#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
 
int main(int argc, const char *argv[])
{
    int s, cs;
    struct sockaddr_in server, client;
    char msg[2000];
     
    // create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
		return -1;
    }
    printf("socket created");
     
    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(12345);
     
    // bind
    if (bind(s,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return -1;
    }
    printf("bind done");
     
    // listen
    listen(s, 3);
    printf("waiting for incoming connections...\n");
     
    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    if ((cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) {
        perror("accept failed");
        return -1;
    }
    printf("connection accepted\n");
     
	int msg_len = 0;
    // receive a message from client
    while ((msg_len = recv(cs, msg, sizeof(msg), 0)) > 0) {
        // send the message back to client
        write(cs, msg, msg_len);
    }
     
    if (msg_len == 0) {
        printf("client disconnected\n");
    }
    else { // msg_len < 0
        perror("recv failed");
		return -1;
    }
     
    return 0;
}
