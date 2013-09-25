int start_peer_connection(int argc, char *argv[]);
int handshake_message();
void requestblock_message(int index, int begin, int length);
void requestblock_message();
void piece_message(int len, unsigned char *buf);
int process_message_loop();