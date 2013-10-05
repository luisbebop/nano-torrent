#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bencode.h"
#include "net.h"
#include "util.h"
#include "sha1.h"

// The SHA1 digest of the .torrent we're downloading this time.
unsigned char digest[20];

// we generate a new random peer id every time we start
char peer_id[21]="-NYANCAT-";

// SHA1 from pieces inside .torrent file
unsigned char pieces_sha1[2048];

// buffer to store a full piece from block requests. supports .torrent pieces til 32k
unsigned char piece_buf[32 * 1024];

// length of pieces to download
int piece_length;

// next piece to download
int requested_piece;

// name of the file we are downloading
char downloading_filename[32];

// length of the file we are downloading
int downloading_filelen;

// block size from tcp socket receive
#define SIZE_RECV 1024

// read a torrent file. Support torrent with size less than 12k
// 0 = read or parser error
// 1 = file read ok
int parse_torrent(char *torrent_filename) {
	struct bencode *torrent;
	struct bencode *info;
	struct bencode_list *files;
	struct bencode_str *filename;	
	unsigned char buf[12 * 1024];
	unsigned char *sha1_pieces;
	int sha1_pieces_len;
	int len = 0;
	size_t info_len;
	char *info_encoded;
	char display[41];
	
	// read a torrent file
	memset(buf, 0, sizeof(buf));
	len = readbinaryfile(buf, torrent_filename);
	// read fail
	if (len <= 0) {
		return 0;
	}
	
	// decode the bencode buffer
	torrent = (struct bencode*)ben_decode(buf,len);
	// decode fail
	if(!torrent) {
		return 0;
	}
	
	// pull out the .info part, which has stuff about the file we're downloading
	info = (struct bencode*)ben_dict_get_by_str((struct bencode*)torrent,"info");
	// decode fail
	if(!info) {
		// clean memory and return
		ben_free(torrent);
		return 0;
	}
	
	// compute the message digest and info_hash from the "info" field in the torrent 
	info_encoded = ben_encode(&info_len,(struct bencode*)info);
	sha1_compute(info_encoded, info_len, digest);
	free(info_encoded);
	memset(display, 0, sizeof(display));
	sha1_hexstring(digest, display);
	printf("sha1 info .torrent=%s\n", display);
	
	// get the piece length
	piece_length = ((struct bencode_int*)ben_dict_get_by_str(info,"piece length"))->ll;
	printf("parse_torrent piece_length=%d\n", piece_length);
	
	// get the concatened SHA1 from all pieces
	sha1_pieces = ((struct bencode_str*)ben_dict_get_by_str(info,"pieces"))->s;
	sha1_pieces_len = ((struct bencode_str*)ben_dict_get_by_str(info,"pieces"))->len;
	memcpy(pieces_sha1, sha1_pieces, sha1_pieces_len);
	hexdump(pieces_sha1, sha1_pieces_len, "pieces_sha1");
	
	// get the files dict from multiple file torrent. otherwise is a single file torrent
	files = (struct bencode_list*)ben_dict_get_by_str(info,"files");
	if (files) {
		printf("parse_torrent multiple files\n");
	}
	else {
		filename = (struct bencode_str*)ben_dict_get_by_str(info,"name");
		memset(downloading_filename,0,sizeof(downloading_filename));
		strcpy(downloading_filename,filename->s);
		downloading_filelen = ((struct bencode_int*)ben_dict_get_by_str(info,"length"))->ll;
		requested_piece = check_next_piece(downloading_filename, downloading_filelen, piece_length);
		
		printf("parse_torrent downloading_filename=%s\n", downloading_filename);
		printf("parse_torrent downloading_filelen=%d\n", downloading_filelen);
		printf("parse_torrent requested_piece=%d\n", requested_piece);
	}
			
	// clean memory and return
	ben_free(torrent);
	return 1;
}

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
	int piece = MAKELONG(MAKEWORD(buf[3], buf[2]), MAKEWORD(buf[1], buf[0]));
	int offset = MAKELONG(MAKEWORD(buf[7], buf[6]), MAKEWORD(buf[5], buf[4]));
	int datalen = len-8;
	int maxlen = 0;
	int size_to_receive = SIZE_RECV;
	unsigned char sha1_piece[20];
	FILE *fp;
	
	printf("piece_message len=%d\n", datalen);
	printf("piece_message piece=%d\n", piece);
	printf("piece_message requested_piece=%d\n", requested_piece);
	printf("piece_message offset=%d\n", offset);

	memcpy(&piece_buf[offset], &buf[8], datalen);
	
	// just get a new piece or the last piece from file that may contain bytes from the
	// first piece of another file
	offset+=datalen;
	printf("piece_message download= %d|%d\n", piece*piece_length+offset, downloading_filelen);
	if (offset == piece_length || (offset+(piece*piece_length) == downloading_filelen)) {
		// check SHA1 from this piece. just write to file if the SHA1 from this piece is valid
		sha1_compute(piece_buf, offset, sha1_piece);
		if (memcmp(&sha1_piece[0], &pieces_sha1[piece*20], 20) == 0) {
			printf("piece_message SHA1 valid. writing piece_buf\n");
			// write this piece to a file
			fp = fopen(downloading_filename,"ab");
			fwrite(piece_buf, offset, 1, fp);
			fclose(fp);
		}
		
		// reset offset for the next piece
		offset = 0;
	}
	
	// request a new piece. if check_next_piece == -1, download finished and don't request a new piece
	requested_piece = check_next_piece(downloading_filename, downloading_filelen, piece_length);
	printf("piece_message requested_piece=%d\n", requested_piece);
	if (requested_piece >= 0) {
		// the last block is likely to be of non-standard size
		maxlen = downloading_filelen - (piece*piece_length+offset);
		if (maxlen < SIZE_RECV) {
			size_to_receive = maxlen;
		}
		printf("piece_message size_to_receive=%d\n", size_to_receive);
		requestblock_message(requested_piece,offset,size_to_receive);
	}	
}

// open a file checking what is the next piece to download. return the index of the piece to download
// or -1 if the file is already downloaded.
int check_next_piece(char * filename, int length, int piece_length) {
	int file_length = 0;
	
	file_length = getfilesize(filename);
	printf("check_next_piece file_length=%d\n", file_length);
	// file doesn't exist or it hasn't any piece written yet.
	if (file_length <= 0) {
		return 0;
	}
	// size from file written on disk is equal size from file inside .torrent info
	if (file_length == length) {
		return -1;
	}
	// divides the actual size and the piece_length to check the next piece
	// in a common torrent implementation we have to download pieces in a random order
	return file_length/piece_length;
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
				requestblock_message(requested_piece,0,SIZE_RECV);
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