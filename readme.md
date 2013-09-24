A small (nano) torrent client with the purpose to download files from torrent peers without serving pieces or opening multiple sockets.
This can be used in embedded environments with sockets restrictions and low bandwidth.

Compiling
	$ gcc -o nano-torrent main.c
	
Usage
	$ ./nano-torrent localhost 23174
	
Working in progress.

Additional links
	https://wiki.theory.org/BitTorrentSpecification#Handshake
	http://en.wikipedia.org/wiki/Torrent_file