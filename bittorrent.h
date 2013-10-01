#ifndef _BITTORRENT_H
#define _BITTORRENT_H

int parse_torrent(char * torrent_filename);
int start_peer_connection(int argc, char *argv[]);
int handshake_message();
void requestblock_message(int index, int begin, int length);
void requestblock_message();
void piece_message(int len, unsigned char *buf);
int check_next_piece(char * filename, int length, int piece_length);
int process_message_loop();

#endif /* _BITTORRENT_H */