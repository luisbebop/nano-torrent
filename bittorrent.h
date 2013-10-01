#ifndef _BITTORRENT_H
#define _BITTORRENT_H

int start_peer_connection(int argc, char *argv[]);
int handshake_message();
void requestblock_message(int index, int begin, int length);
void requestblock_message();
void piece_message(int len, unsigned char *buf);
void parse_torrent(char * torrent_filename);
int process_message_loop();

#endif /* _BITTORRENT_H */