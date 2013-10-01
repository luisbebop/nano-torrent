#ifndef _UTIL_H
#define _UTIL_H

#define MAKEWORD(a, b)      ((unsigned short)(((unsigned char)((a) & 0xff)) | ((unsigned short)((unsigned char)((b) & 0xff))) << 8))
#define MAKELONG(low,high)	((int)(((unsigned short)(low)) | (((unsigned int)((unsigned short)(high))) << 16)))
#define LOWORD(l)           ((unsigned short)((l) & 0xffff))
#define HIWORD(l)           ((unsigned short)((l) >> 16))
#define LOBYTE(w)           ((unsigned char)((w) & 0xff))
#define HIBYTE(w)           ((unsigned char)((w) >> 8))
#define HH(x)				HIBYTE(HIWORD( x ))
#define HL(x)				LOBYTE(HIWORD( x ))
#define LH(x)				HIBYTE(LOWORD( x ))
#define LL(x)				LOBYTE(LOWORD( x ))
#define LONIBLE(x)			(((unsigned char)x) & 0x0F )
#define HINIBLE(x)			((((unsigned char)x) * 0xF0)>>4)

void hexdump(unsigned char *data, int size, char *caption);
int readbinaryfile(unsigned char * buffer, char * filename);
int getfilesize(char *filename);

#endif /* _UTIL_H */