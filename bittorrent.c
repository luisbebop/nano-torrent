#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "bittorrent.h"
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

// number of pieces to download
int pieces_num;

// next piece to download
int requested_piece;

// .torrent dictionary
struct bencode *torrent;

// .torrent info dictionary
struct bencode *info;

// .torrent files dictionary
struct bencode_list *files;

// length of the file being downloaded
int file_length;

// block size from tcp socket receive
#define SIZE_RECV 1024

// read a torrent file. Support torrent with size less than 12k
// 0 = read or parser error
// 1 = file read ok
int parse_torrent(char *torrent_filename) {	
	unsigned char buf[12 * 1024];
	unsigned char *sha1_pieces;
	int sha1_pieces_len;
	int len = 0;
	size_t info_len;
	char *info_encoded;
	
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
		return 0;
	}
	
	// compute the message digest and info_hash from the "info" field in the torrent 
	info_encoded = ben_encode(&info_len,(struct bencode*)info);
	sha1_compute(info_encoded, info_len, digest);
	free(info_encoded);
	hexdump(digest, 20, "parse_torrent digest");
	
	// get the piece length
	piece_length = ((struct bencode_int*)ben_dict_get_by_str(info,"piece length"))->ll;
	printf("parse_torrent piece_length=%d\n", piece_length);
	
	// get the concatened SHA1 from all pieces
	sha1_pieces = ((struct bencode_str*)ben_dict_get_by_str(info,"pieces"))->s;
	sha1_pieces_len = ((struct bencode_str*)ben_dict_get_by_str(info,"pieces"))->len;
	memcpy(pieces_sha1, sha1_pieces, sha1_pieces_len);
	pieces_num = (sha1_pieces_len/20);
	printf("parse_torrent pieces=%d\n", pieces_num);
	hexdump(pieces_sha1, sha1_pieces_len, "parse_torrent pieces_sha1");
	
	// checking next piece to download
	requested_piece = check_next_piece();
	printf("parse_torrent requested_piece=%d\n", requested_piece);
	printf("parse_torrent file_length=%d\n", file_length);
				
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
	
	printf("piece_message len=%d\n", datalen);
	printf("piece_message piece=%d\n", piece);
	printf("piece_message requested_piece=%d\n", requested_piece);
	printf("piece_message offset=%d\n", offset);

	memcpy(&piece_buf[offset], &buf[8], datalen);
	
	// just get a new piece or the last piece from file that may contain bytes from the
	// first piece of another file
	offset+=datalen;
	
	printf("piece_message download= %d|%d\n", offset+(piece*piece_length), file_length);
	if (offset == piece_length || (offset+(piece*piece_length) == file_length)) {
		// check SHA1 piece
		if(check_sha1_piece(piece_buf, piece, offset)) {
			// saving piece to disk
			save_piece(piece_buf, piece, offset);	
		}
		// reset offset for the next piece
		offset = 0;
	}
			
	// request a new piece. if check_next_piece == -1, download finished and don't request a new piece
	requested_piece = check_next_piece();
	printf("piece_message requested_piece=%d\n", requested_piece);
	if (requested_piece >= 0) {
		// the last block is likely to be of non-standard size
		maxlen = file_length - (piece*piece_length+offset);
		if (maxlen < SIZE_RECV) {
			size_to_receive = maxlen;
		}
		printf("piece_message size_to_receive=%d\n", size_to_receive);
		requestblock_message(requested_piece,offset,size_to_receive);
	}	
}

int check_sha1_piece(unsigned char *piece_buf, int piece, int offset) {
	unsigned char sha1_piece[20];
	
	// check SHA1 from this piece. just write to file if the SHA1 from this piece is valid
	sha1_compute(piece_buf, offset, sha1_piece);
	if (memcmp(&sha1_piece[0], &pieces_sha1[piece*20], 20) == 0) {
		return 1;
	} else {
		printf("invalid sha1 piece\n");		
		return 0;
	}
}

void save_piece(unsigned char *piece_buf, int piece, int offset) {
	struct bencode* file;
	struct bencode_list* path;
	int i;
	int index;
	int bytes_to_write;
	int file_len;
	int file_len_disk;
	FILE *fp;
		
	if (files) {
		printf("save_piece multiple files piece=%d\n", piece);
		bytes_to_write = offset;
		index = 0;
				
		for (i=0; i < files->n; i++) {
			file = files->values[i];
			path = (struct bencode_list*)ben_dict_get_by_str(file,"path");
			file_len = ((struct bencode_int*)ben_dict_get_by_str(file,"length"))->ll;
			file_len_disk = getfilesize(((struct bencode_str*)path->values[(path->n)-1])->s);
			if (file_len_disk < 0) {
				file_len_disk = 0;
			}
			
			printf("[%d]save_piece filename=%s\n", i, ((struct bencode_str*)path->values[(path->n)-1])->s);
			printf("[%d]save_piece file_len=%d\n", i, file_len);
			printf("[%d]save_piece bytes_to_write=%d\n", i, bytes_to_write);
			printf("[%d]save_piece file_len-file_len_disk=%d\n", i, file_len - file_len_disk);
			
			if (file_len_disk < file_len && bytes_to_write > 0) {
				// check file buffer cross boundaries
				if (bytes_to_write > (file_len - file_len_disk)) {
					printf("[%d]save_piece cross-boundaries index=%d bytes_to_write=%d\n", i, index, bytes_to_write);
					fp = fopen(((struct bencode_str*)path->values[(path->n)-1])->s,"ab");
					fwrite(&piece_buf[index], (file_len - file_len_disk), 1, fp);
					fclose(fp);
					index += (file_len - file_len_disk);
					bytes_to_write -= (file_len - file_len_disk);
					printf("[%d]save_piece cross-boundaries index=%d bytes_to_write=%d\n", i, index, bytes_to_write);					
				} else {
					printf("[%d]save_piece normal index=%d bytes_to_write=%d\n", i, index, bytes_to_write);	
					fp = fopen(((struct bencode_str*)path->values[(path->n)-1])->s,"ab");
					fwrite(&piece_buf[index], bytes_to_write, 1, fp);
					fclose(fp);
					return;
				}
			}
		}
	} else {
		printf("save_piece single file\n");
		// write this piece to a file
		fp = fopen(((struct bencode_str*)ben_dict_get_by_str(info,"name"))->s,"ab");
		fwrite(piece_buf, offset, 1, fp);
		fclose(fp);
	}
}

// open a file checking what is the next piece to download. return the index of the piece to download
// or -1 if the file is already downloaded.
int check_next_piece() {
	int i = 0;
	int file_len_disk = 0;
	int total_size = 0;
	struct bencode* file;
	struct bencode_list* path;

	// get the files dict from multiple file torrent. otherwise is a single file torrent
	files = (struct bencode_list*)ben_dict_get_by_str(info,"files");
	
	if (files) {
		file_length = 0;
		printf("check_next_piece multiple files\n");
		for (i=0; i < files->n; i++) {
			file = files->values[i];
			path = (struct bencode_list*)ben_dict_get_by_str(file,"path");
			printf("check_next_piece filename[i]=%s\n", ((struct bencode_str*)path->values[(path->n)-1])->s);
			
			file_length += ((struct bencode_int*)ben_dict_get_by_str(file,"length"))->ll;
			file_len_disk = getfilesize(((struct bencode_str*)path->values[(path->n)-1])->s);
		
			printf("check_next_piece file_len_disk=%d\n", file_len_disk);
		
			// sum the size of all files on disk
			if (file_len_disk > 0) {
				total_size += file_len_disk;
			}
		}
	} else {
		printf("check next_piece single file\n");
		
		// set the size from file inside .torrent
		file_length = ((struct bencode_int*)ben_dict_get_by_str(info,"length"))->ll;
		
		// get the filename from file inside .torrent and check its size
		total_size = getfilesize(((struct bencode_str*)ben_dict_get_by_str(info,"name"))->s);
		printf("check_next_piece file_length=%d\n", file_length);
	}
	
	// size from file written on disk is equal size from file inside .torrent info
	if (total_size == file_length) {
		return -1;
	}

	// file doesn't exist or it hasn't any piece written yet.
	if (total_size <= 0) {
		return 0;
	}

	// divides the actual size and the piece_length to check the next piece
	// in a common torrent implementation we have to download pieces in a random order
	return total_size/piece_length;
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

void clean_memory() {
	if(torrent) {
		ben_free(torrent);
	}
}