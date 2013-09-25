void error(const char *msg);
void uclconnect(int argc, char *argv[]);
int uclsend(unsigned char * buf, int len);
int uclreceive(unsigned char * buf, int maxlen);
void uclclose();