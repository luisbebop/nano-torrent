#include <stdio.h>
#include <string.h>
#include "bencode.h"
#include "net.h"
#include "util.h"

// The SHA1 digest of the document we're downloading this time.
unsigned char digest[20] = {0xcb, 0xe2, 0xe7, 0xda, 0x3d, 0xa5, 0x90, 0x4b, 0xb4, 0xb6, 0x2b, 0x78, 0x23, 0x2e, 0xeb, 0x20, 0xbc, 0x2e, 0x35, 0xc9};

// we generate a new random peer id every time we start
char peer_id[21]="-NYANCAT-";

// try to connect to a peer and init a bittorrent session
// connection error or messages error = 0
// download complete from the digest torrent = 1
int start_peer_connection(int argc, char *argv[]) {
	int ret;
	// connect to a peer
	uclconnect(argc,argv);
	
	if (handshake_message()) {
		return process_message_loop();
	}
	
	return 0;
}

// exchange a bittorrent protocol handshake message with the connected peer
// 0 = handshake fail 1 = handshake success
int handshake_message() {
	char protocol[] = "BitTorrent protocol";
	unsigned char pstrlen = strlen(protocol);
	unsigned char buf[pstrlen+49];
	unsigned char recv[pstrlen+49];
	int recvd;
	
	// build handshake
	buf[0]=pstrlen;
	memcpy(&buf[1],protocol,pstrlen);
	memcpy(&buf[1+pstrlen+8],digest,20);
	memcpy(&buf[1+pstrlen+8+20],peer_id,20);
	// send handshake
	uclsend(buf, sizeof(buf));
	hexdump(buf, sizeof(buf), "sent handshake");
	// receive handshake
	recvd = uclreceive(recv, pstrlen+49);
	hexdump(recv, recvd, "received handshake");
	
	if (recvd != pstrlen+49) {
		return 0;
	}
	
	return 1;
}

void interested_message() {
	int msgsize = 1;
	unsigned char buf[5] = {HH(msgsize), HL(msgsize), LH(msgsize), LL(msgsize), 0x02};
	
	// send interested
	uclsend(buf, 5);
	hexdump(buf, 5, "sent interested");
}

void requestblock_message(int index, int begin, int length) {
	int msgsize = 13;
	unsigned char buf[17] = {HH(msgsize), HL(msgsize), LH(msgsize), LL(msgsize), 0x06,
							 HH(index), HL(index), LH(index), LL(index),
							 HH(begin), HL(begin), LH(begin), LL(begin),
							 HH(length), HL(length), LH(length), LL(length)}; 
	// send request block message
	uclsend(buf, 17);
	hexdump(buf, 17, "sent request block");
}

void piece_message(int len, unsigned char *buf) {
	//hexdump(buf, len, "index+begin+blockdata");
}

void parse_torrent(char * torrent_filename) {
	// struct bencode_dict *torrent;
	// struct bencode *info;
	// struct bencode_list* files = (struct bencode_list*)ben_dict_get_by_str(info,"files");
	// int piece_length = 0;
	// 
	// if (files) {
	// 	printf("parse_torrent files != 0 ... multiple files\n");
	// }
	// else {
	// 	printf("parse_torrent files == 0 ... single file\n");
	// }
	// 
	// piece_length = ((struct bencode_int*)ben_dict_get_by_str(info,"piece length"))->ll;
	// printf("parse_torrent piece_length=%d\n", piece_length);
	unsigned char buf[4096];
	int len = 0;
	
	memset(buf, 0, sizeof(buf));
	len = readbinaryfile(buf, torrent_filename);
	if (len > 0) {
		hexdump(buf, len, "readbinaryfile");
	} else {
		printf("error on read torrent file.\n");
	}
}

// a main loop tha process all bittorrent messages received from the peer
// 0 = message error
// 1 = finished with that peer (probably completed the file download!)
int process_message_loop() {
	// recv buffer
	unsigned char recv[2048];
	int recvd = 0;
	int size_to_receive;
	
	while(42) {
		// clean recv buffer
		memset(recv,0,sizeof(recv));
		
		printf("trying to receive a message ...\n");
		
		// receive 4 bytes of message size
		recvd = uclreceive(recv, 4);
		hexdump(recv, recvd, "uclreceive 4");
		if (recvd != 4) {
			// close connection and return
			printf("closing connection and exiting process_message_loop()\n");
			// close peer
			uclclose();
			return 0;
		}
		
		// convert 4 bytes of message size to an int
		size_to_receive = MAKELONG(MAKEWORD(recv[3], recv[2]), MAKEWORD(recv[1], recv[0]));
		printf("size_to_receive=%d\n", size_to_receive);
		
		// is keep alive message?
		if (size_to_receive == 0) {
			printf("keep alive message received\n");
			continue;
		}
		
		// receive the next bytes from another messages
		recvd = uclreceive(recv, size_to_receive);
		hexdump(recv, recvd, "uclreceive N");
		if (recvd != size_to_receive) {
			// close connection and return
			printf("closing connection and exiting process_message_loop()\n");
			// close peer
			uclclose();
			return 0;
		}
				
		// parse messages
		switch(recv[0]) {
			case 1: {
				hexdump(recv, recvd, "unchoke message received");
				requestblock_message(0,0,1024);
				break;
			}
			case 5: {
				hexdump(recv, recvd, "bitfield message received");
				interested_message();
				break;
			}
			case 7: {
				hexdump(recv, recvd, "piece message received");
				piece_message(recvd-1, &recv[1]);
				break;
			}
		}
	}
}