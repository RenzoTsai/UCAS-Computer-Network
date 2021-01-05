#include "tcp_sock.h"

#include "log.h"

#include <unistd.h>
#include<stdio.h>

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

	int rbuf_size = 5*1024*1024;
	FILE *f = fopen("server-output.dat","w");
	char* rbuf = (char*)malloc(rbuf_size);//5MB
	int rlen=0;
	int total_recv = 0;
	while(1){
		memset(rbuf, 0, rbuf_size);
		rlen = tcp_sock_read(csk, rbuf, rbuf_size);
		if (rlen <= 0) {
			log(DEBUG, "tcp_sock_read return 0 value, finish transmission.");
			break;
		} 
		else if (rlen > 0) {
			rbuf[rlen] = '\0';
			fprintf(f, "%s", rbuf);
			total_recv += rlen;
			fprintf(stdout, "totally receive data: %d B\n", total_recv);
		}
	}
	fclose(f);

	log(DEBUG, "close this connection.");

	tcp_sock_close(csk);
	
	return NULL;
}

// tcp client application, connects to server (ip:port specified by arg), each
// time sends one bulk of data and receives one bulk of data 
void *tcp_client(void *arg)
{
	struct sock_addr *skaddr = arg;

	FILE *f = fopen("client-input.dat","r");
	char* wbuf = (char*)malloc(5*1024*1024);
	fseek(f,0,0);
	int wlen = fread(wbuf, sizeof(char), 5*1024*1024, f);
	fclose(f);

	struct tcp_sock *tsk = alloc_tcp_sock();

	if (tcp_sock_connect(tsk, skaddr) < 0) {
		log(ERROR, "tcp_sock connect to server ("IP_FMT":%hu)failed.", \
				NET_IP_FMT_STR(skaddr->ip), ntohs(skaddr->port));
		exit(1);
	}

	if(tcp_sock_write(tsk, wbuf, wlen)<0){
		fprintf(stdout,"********** tcp_sock_write_error ****************");
	}
    sleep(5);
	fprintf(stdout,"send over\n");
	tcp_sock_close(tsk);

	return NULL;
}
