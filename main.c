#include <stdio.h>
#include <string.h>
#include "bittorrent.h"

int main(int argc, char *argv[]) {
	int ret;
	
	// banner
	printf("nano-torrent\n");
	
	// debug torrent file
	ret = parse_torrent("beauty_in_perspective.torrent");
	printf("parse_torrent ret=%d\n", ret);

	// connect to a peer and starting to process bittorrent messages
	ret = start_peer_connection(argc,argv);
	printf("start_peer_connection ret=%d\n", ret);

	return 0;
}