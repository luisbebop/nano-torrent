A small (nano) torrent client with the purpose to download files from torrent peers without serving pieces or opening multiple sockets.
This can be used in embedded environments with sockets restrictions and low bandwidth.

Compiling

	$ gcc -o nano-torrent main.c
	
Usage

	$ make
	$ ./all localhost 23174
	
Additional links

	https://wiki.theory.org/BitTorrentSpecification#Handshake
	http://en.wikipedia.org/wiki/Torrent_file