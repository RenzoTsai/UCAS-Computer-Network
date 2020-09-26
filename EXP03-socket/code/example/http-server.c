#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

int msg_handler(char * msg, char * path);
void * handle_request(void * cs);

int main(int argc, const char *argv[])
{
    int s;
    struct sockaddr_in server, client;

    // create socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket failed");
		return -1;
    }
    printf("socket created");
     
    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(80);
     
    // bind
    if (bind(s,(struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return -1;
    }
    printf("bind done");
     
    // listen
    listen(s, 128);
    printf("waiting for incoming connections...");
     
    // accept connection from an incoming client
    int c = sizeof(struct sockaddr_in);
    while(1) {
        int cs;
        pthread_t thread1;
        if ((cs = accept(s, (struct sockaddr *)&client, (socklen_t *)&c)) < 0) {
            perror("accept failed");
            return -1;
        }
        // handle_request(&cs1);
        pthread_create(&thread1, NULL, handle_request, &cs);
        // pthread_create(&thread2, NULL, handle_request, &cs2);

        pthread_detach(thread1);
        //pthread_join(thread1, NULL);
    }

 
    return 0;
}

/* parse file path */
int msg_handler(char * msg, char * path) {
    int i = 0;
    int j = 0;

    for ( ; i < strlen(msg) && msg[i] != ' '; i++) {
    }
    i++;
    if (msg[i]=='/') {
        i++;
    }

    for ( ; i < strlen(msg) && msg[i] != ' '; i++) {
        path[j] = msg[i];
        j++;
    }

    path[j] = '\0';
    return 1;
}

void * handle_request(void * csock) {
    int cs = *(int *)csock;
    printf("connection accepted\n");
    // receive a message from client
    char msg[2000];
    int msg_len = 0;
    char * send_msg_buffer;
    while ((msg_len = recv(cs, msg, sizeof(msg), 0)) > 0) {
        // send the message back to client
        char path[50];              //target file path
        bzero(path, sizeof(path));

        printf("%s\n",msg);
        msg_handler(msg, path);
        printf("path: %s\n",path);
        FILE * fd = fopen(path, "r");

        char return_message[2000];
        bzero(return_message, sizeof(return_message));

        strcat(return_message, "HTTP/1.1 ");
        if(fd == NULL){
            strcat(return_message, "404 Not Found\r\n"); 
        } else { 

            /*read target file */
            fseek(fd, 0, SEEK_END);
            int file_length = ftell(fd);
            char file_len_string[20];
            send_msg_buffer = (char*) malloc(sizeof(char) * file_length);
            fseek(fd, 0, 0);
            fread(send_msg_buffer, file_length + 1, sizeof(char), fd);

            /*add http head */
            strcat(return_message, "200 OK\r\n"); 
            //strcat(return_message, "Content-Type: text/plain\r\n");
            strcat(return_message, "Content-Length: ");
            sprintf(file_len_string,"%d", file_length);
            strcat(return_message, file_len_string);
            strcat(return_message, "\r\n"); 
            strcat(return_message, "Connection: Keep-Alive\r\n");
            strcat(return_message, "\r\n");
            strcat(return_message, send_msg_buffer);
            strcat(return_message, "\r\n");  
            free(send_msg_buffer);
            fclose(fd);
        }

        printf("%s", return_message);
        write(cs, return_message, strlen(return_message));
    }
     
    if (msg_len == 0) {
        printf("client disconnected");
    }
    else { // msg_len < 0
        perror("recv failed");
		exit(-1);
    }
    exit(0);
}