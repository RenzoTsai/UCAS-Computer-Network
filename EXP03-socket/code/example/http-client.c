/* client application */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

char * handle_recv_msg(char * server_reply, char * processed_msg);
 
int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in server;
    char message[1000], server_reply[2000];
     
    // create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("create socket failed\n");
		return -1;
    }
    printf("socket created\n");
     
    server.sin_addr.s_addr = inet_addr("10.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(80);
 
    // connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed");
        return 1;
    }
     
    printf("connected\n");
     
    while(1) {
        char path[30];
        char recv_path[35] = "recv_";
        
        FILE * fd;
        char processed_msg[200];

        printf("enter message : ");
        scanf("%s", path);
        getchar();
        strcat(recv_path,path);

        bzero(message, sizeof(message));
        strcat(message, "GET /");
        strcat(message, path);
        strcat(message, " HTTP/1.1\r\n");
        //strcat(message, "User-Agent: Wget/1.17.1 (linux-gnu\r\n");
        strcat(message, "Accept: */*\r\n"); 
        strcat(message, "Accept-Encoding: identity\r\n"); 
        strcat(message, "Host: 10.0.0.1\r\n"); // HTTP 1.1 needs to add host.
        strcat(message, "Connection: Keep-Alive\r\n");
        strcat(message, "\r\n");
        // strcat(message, "\r\n");
        // strcat(message, "\r\n");

        printf("send msg:\n%s\n", message);
        
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
        printf("len : %d \nserver reply : ", len);
        printf("%s\n", server_reply);

        if(strstr(server_reply, "404 Not Found")){
            printf("HTTP 404 Not Found\n");
            continue;
        }
        fd = fopen(recv_path, "wb+");
        
        if (handle_recv_msg(server_reply, processed_msg) != NULL) {
            fwrite(processed_msg, 1, strlen(processed_msg), fd);
        } else {
            printf("save failed\n");
        }

        char * result;
        if ((result = strtok(server_reply,"\r\n")) != NULL) {
            printf("%s\n", result);
        }


        fclose(fd);
    }
     
    close(sock);
    return 0;
}

char * handle_recv_msg(char * server_reply, char * processed_msg) {
    int cursor = 0;
    int flag = 0;
    
    for( ; cursor < strlen(server_reply) - 3 ; cursor++) {
        if (server_reply[cursor] == '\r'     && server_reply[cursor + 1] == '\n' 
         && server_reply[cursor + 2] == '\r' && server_reply[cursor + 3] == '\n') {
            flag = 1;
            break;
        }
    }

    if (!flag) {
        return NULL;
    }
    cursor = cursor + 4;
    if(cursor < strlen(server_reply)) {
        strcpy(processed_msg, &server_reply[cursor]);
        return &server_reply[cursor];
    } else {
        return NULL;
    }
    
}
