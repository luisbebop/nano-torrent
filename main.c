#include <stdio.h>
#include <string.h>
#include "bittorrent.h"

int main(int argc, char *argv[]) {
	int ret;
	
	// banner
	printf("nano-torrent\n");
	
	// debug torrent file
	//ret = parse_torrent("beauty_in_perspective.torrent");
	//ret = parse_torrent("test-torrent.torrent");
	//ret = parse_torrent("pale-blue-dot.torrent");
	ret = parse_torrent("walk_update.torrent");
	
	printf("parse_torrent ret=%d\n", ret);

	// connect to a peer and starting to process bittorrent messages
	ret = start_peer_connection(argc,argv);
	printf("start_peer_connection ret=%d\n", ret);
	
	// clean memory from bencode parser
	clean_memory();

	return 0;
}