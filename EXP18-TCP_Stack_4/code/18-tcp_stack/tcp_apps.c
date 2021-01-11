#include "tcp_sock.h"

#include "log.h"

#include <unistd.h>
#include<stdio.h>

#define SENT_SIZE 1350880

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

	int rbuf_size = SENT_SIZE + 1;
	FILE * fd = fopen("server-output.dat","w");
	char * rbuf = (char*)malloc(rbuf_size);
	int rlen = 0;
	int recv_len = 0;
	while (1) {
		memset(rbuf, 0, rbuf_size);
		rlen = tcp_sock_read(csk, rbuf, rbuf_size);
		if (rlen <= 0) {
			log(DEBUG, "tcp_sock_read return 0, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			fprintf(fd, "%s", rbuf);
			recv_len += rlen;
			fprintf(stdout, "received data: %d B\n", recv_len);
			if (rlen < 1460) {
				printf("%s\n",rbuf);
				break;
			}
		}
	}
	fclose(fd);

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	FILE * fd = fopen("client-input.dat","r");
	char * wbuf = (char*)malloc(SENT_SIZE + 1);
	fseek(fd,0,0);
	int wlen = fread(wbuf, sizeof(char), SENT_SIZE, fd);
	fclose(fd);

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

	if(tcp_sock_write(tsk, wbuf, wlen) < 0){
		fprintf(stdout,"tcp_sock_write_error.");
	}

	tcp_sock_close(tsk);

	return NULL;
}
