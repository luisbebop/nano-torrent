#ifndef _NET_H
#define _NET_H

void error(const char *msg);
void uclconnect(int argc, char *argv[]);
int uclsend(unsigned char * buf, int len);
int uclreceive(unsigned char * buf, int maxlen);
void uclclose();

#endif /* _NET_H */