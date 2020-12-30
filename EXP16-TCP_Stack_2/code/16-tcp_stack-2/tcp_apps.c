#include "tcp_sock.h"

#include "log.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LEN 1000

// tcp server application, listens to port (specified by arg) and serves only one
// connection request
void *tcp_server(void *arg)
{
	u16 port = *(u16 *)arg;
	struct tcp_sock *tsk = alloc_tcp_sock();

	struct sock_addr addr;
	addr.ip = htonl(0);
	addr.port = port;
	if (tcp_sock_bind(tsk, &addr) < 0) {
		log(ERROR, "tcp_sock bind to port %hu failed", ntohs(port));
		exit(1);
	}

	if (tcp_sock_listen(tsk, 3) < 0) {
		log(ERROR, "tcp_sock listen failed");
		exit(1);
	}

	log(DEBUG, "listen to port %hu.", ntohs(port));

	struct tcp_sock *csk = tcp_sock_accept(tsk);

	log(DEBUG, "accept a connection.");

	char rbuf[MAX_LEN + 1];
	char wbuf[1024];
	int rlen = 0;
	int time = 0;
    FILE *fd;
	fd = fopen("server-output.dat","wb");
	int file_ptr = 0;
	while (1) {
		rlen = tcp_sock_read(csk, rbuf, MAX_LEN);
		if (rlen == 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			sprintf(wbuf, "server echoes: %d", time++);
			fseek(fd, file_ptr, 0);
            int wsize = fwrite(rbuf, 1, rlen, fd);
			file_ptr += wsize;
			printf("ptr:%d\n",file_ptr);
			if (wsize < MAX_LEN) {
				printf("\nwsize:%d, len:%d\n%s\n",wsize, rlen, rbuf);
				fclose(fd);
				break;
			}
			if (tcp_sock_write(csk, wbuf, strlen(wbuf)) < 0) {
				log(DEBUG, "tcp_sock_write return negative value, something goes wrong.");
				exit(1);
			}
		}
		else {
			fprintf(stdout,"%d \n" , rlen);
			log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
			exit(1);
		}
	}

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}
	
    FILE *fd = fopen("client-input.dat", "r");
	char c;
	char *wbuf = (char *)malloc(20000000);
	
	int wlen = 0;
	while((c = fgetc(fd)) != EOF) { 
        wbuf[wlen] = c;
        wlen++;
    }
	char rbuf[MAX_LEN + 1];
	int rlen = 0;

	int n = wlen / MAX_LEN + 1;
	for (int i = 0; i < n; i++) {
        if(wlen >= MAX_LEN){
		    if (tcp_sock_write(tsk, wbuf + i * MAX_LEN, MAX_LEN) < 0) {
			    break;
       		}
		} else if (wlen < MAX_LEN && wlen > 0) {
		    if (tcp_sock_write(tsk, wbuf + i * MAX_LEN, wlen) < 0) {
				break;
			}    
		} else if(wlen <= 0) {
			tcp_sock_write(tsk, wbuf, 0);
			break;
		}

		wlen = wlen - MAX_LEN;
		printf("remained wlen:%d\n",wlen);
		if (wlen <= 0) {
			break;
		}
		
		rlen = tcp_sock_read(tsk, rbuf, MAX_LEN);
		if (rlen == 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		}
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			//fprintf(stdout, "%s\n", rbuf);
		}
		else {
			log(DEBUG, "tcp_sock_read return negative value, something goes wrong.");
			exit(1);
		}

		usleep(500);
	}
	tcp_sock_close(tsk);

	return NULL;
}