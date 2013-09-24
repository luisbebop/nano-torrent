#include <stdio.h>
#include <string.h>
#include "net.h"
#include "util.h"

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
	UCLConnect(argc,argv);

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
	UCLClose();

	return 0;
}