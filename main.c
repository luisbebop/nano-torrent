#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

// socket handle
int socket_handle;

void hex_dump(unsigned char *data, int size, char *caption)
{
	int i; // index in data...
	int j; // index in line...
	char temp[8];
	char buffer[128];
	char *ascii;

	memset(buffer, 0, 128);

	printf("---------> %s <--------- (%d bytes from %p)\n", caption, size, data);

	// Printing the ruler...
	printf("        +0          +4          +8          +c            0   4   8   c   \n");

	// Hex portion of the line is 8 (the padding) + 3 * 16 = 52 chars long
	// We add another four bytes padding and place the ASCII version...
	ascii = buffer + 58;
	memset(buffer, ' ', 58 + 16);
	buffer[58 + 16] = '\n';
	buffer[58 + 17] = '\0';
	buffer[0] = '+';
	buffer[1] = '0';
	buffer[2] = '0';
	buffer[3] = '0';
	buffer[4] = '0';
	for (i = 0, j = 0; i < size; i++, j++)
	{
		if (j == 16)
		{
			printf("%s", buffer);
			memset(buffer, ' ', 58 + 16);

			sprintf(temp, "+%04x", i);
			memcpy(buffer, temp, 5);

			j = 0;
		}

		sprintf(temp, "%02x", 0xff & data[i]);
		memcpy(buffer + 8 + (j * 3), temp, 2);
		if ((data[i] > 31) && (data[i] < 127))
			ascii[j] = data[i];
		else
			ascii[j] = '.';
	}

	if (j != 0)
		printf("%s", buffer);
}

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

int connect_(int argc, char *argv[])
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

	return sockfd;
}

int UCLSend(unsigned char * buf, int len)
{
	int n;
	
	n = write(socket_handle,buf,len);
	if (n < 0) {
		error("ERROR writing to socket");
	}
	
	return n;
}

int UCLReceive(unsigned char * buf, int maxlen)
{
	int n;
	
	n = read(socket_handle,buf,maxlen);
	if (n < 0) {
		error("ERROR reading from socket");
	}
	
	return n;
}


int main(int argc, char *argv[]) {
	// The SHA1 digest of the document we're downloading this time.
	unsigned char digest[20] = {0xcb, 0xe2, 0xe7, 0xda, 0x3d, 0xa5, 0x90, 0x4b, 0xb4, 0xb6, 0x2b, 0x78, 0x23, 0x2e, 0xeb, 0x20, 0xbc, 0x2e, 0x35, 0xc9};
	// we generate a new random peer id every time we start
	char peer_id[21]="-NYANCAT-";

	// send the handshake message 
	char protocol[] = "BitTorrent protocol";
	unsigned char pstrlen = strlen(protocol);
	unsigned char buf[pstrlen+49];

	// recv buffer
	unsigned char recv[1024];
	int recvd = 0;

	//clean variables
	memset(recv,0,sizeof(recv));

	// banner
	printf("nano-torrent\n");

	// connect to a peer
	socket_handle = connect_(argc,argv);

	// build handshake
	buf[0]=pstrlen;
	memcpy(&buf[1],protocol,pstrlen);
	memcpy(&buf[1+pstrlen+8],digest,20);
	memcpy(&buf[1+pstrlen+8+20],peer_id,20);

	// send handshake
	UCLSend(buf, sizeof(buf));
	hex_dump(buf, sizeof(buf), "sent");

	// receive handshake
	recvd = UCLReceive(recv, 100);
	hex_dump(recv, recvd, "received");

	// close peer
	close(socket_handle);

	return 0;
}