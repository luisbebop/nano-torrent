void error(const char *msg);
void UCLConnect(int argc, char *argv[]);
int UCLSend(unsigned char * buf, int len);
int UCLReceive(unsigned char * buf, int maxlen);
void UCLClose();