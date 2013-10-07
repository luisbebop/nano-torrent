#ifndef _BITTORRENT_H
#define _BITTORRENT_H

int parse_torrent(char * torrent_filename);
int start_peer_connection(int argc, char *argv[]);
int handshake_message();
void requestblock_message(int index, int begin, int length);
void requestblock_message();
void piece_message(int len, unsigned char *buf);
int check_sha1_piece(unsigned char *piece_buf, int piece, int offset);
void save_piece(unsigned char *piece_buf, int piece, int offset);
int check_next_piece();
int process_message_loop();
void clean_memory();

#endif /* _BITTORRENT_H */