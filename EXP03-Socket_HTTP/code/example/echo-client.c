/* client application */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
 
int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000], server_reply[2000];
     
    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("create socket failed");
		return -1;
    }
    printf("socket created");
     
    server.sin_addr.s_addr = inet_addr("10.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);
 
    // connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return 1;
    }
     
    printf("connected\n");
     
    while(1) {
        printf("enter message : ");
        scanf("%s", message);
         
        // send some data
        if (send(sock, message, strlen(message), 0) < 0) {
            printf("send failed");
            return 1;
        }
         
        // receive a reply from the server
		int len = recv(sock, server_reply, 2000, 0);
        if (len < 0) {
            printf("recv failed");
            break;
        }
		server_reply[len] = 0;
         
        printf("server reply : ");
        printf("%s\n", server_reply);
    }
     
    close(sock);
    return 0;
}
