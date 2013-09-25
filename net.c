#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>

// socket handle. no simultaneos sockets allowed
int socket_handle;

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

void uclconnect(int argc, char *argv[])
{
	int sockfd, portno, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	if (argc < 3) {
		printf("usage %s hostname port\n", argv[0]);
		exit(0);
	}
	
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		error("ERROR opening socket");
	}
	
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		printf("ERROR, no such host\n");
		exit(0);
	}
	
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);
	if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		error("ERROR connecting");
	}

	socket_handle = sockfd;
}

int uclsend(unsigned char * buf, int len)
{
	int n;
	
	n = write(socket_handle,buf,len);
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	return n;
}

int uclreceive(unsigned char * buf, int maxlen)
{
	int n;
	
	n = read(socket_handle,buf,maxlen);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	return n;
}

void uclclose() {
	close(socket_handle);
}