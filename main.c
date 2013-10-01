#include <stdio.h>
#include <string.h>
#include "bittorrent.h"

int main(int argc, char *argv[]) {
	int ret;
	
	// banner
	printf("nano-torrent\n");
	
	// debug torrent file
	parse_torrent("beauty_in_perspective.torrent");

	// connect to a peer and starting to process bittorrent messages
	ret = start_peer_connection(argc,argv);
	printf("nano-torrent result=%d\n", ret);

	return 0;
}