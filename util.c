#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void hexdump(unsigned char *data, int size, char *caption) {
	int i; // index in data...
	int j; // index in line...
	char temp[8];
	char buffer[128];
	char *ascii;

	memset(buffer, 0, 128);

	printf("---------> %s <--------- (%d bytes from %p)\n", caption, size, data);

	// Printing the ruler...
	printf("        +0          +4          +8          +c            0   4   8   c   \n");

	// Hex portion of the line is 8 (the padding) + 3 * 16 = 52 chars long
	// We add another four bytes padding and place the ASCII version...
	ascii = buffer + 58;
	memset(buffer, ' ', 58 + 16);
	buffer[58 + 16] = '\n';
	buffer[58 + 17] = '\0';
	buffer[0] = '+';
	buffer[1] = '0';
	buffer[2] = '0';
	buffer[3] = '0';
	buffer[4] = '0';
	for (i = 0, j = 0; i < size; i++, j++) {
		if (j == 16) {
			printf("%s", buffer);
			memset(buffer, ' ', 58 + 16);

			sprintf(temp, "+%04x", i);
			memcpy(buffer, temp, 5);

			j = 0;
		}

		sprintf(temp, "%02x", 0xff & data[i]);
		memcpy(buffer + 8 + (j * 3), temp, 2);
		if ((data[i] > 31) && (data[i] < 127)) {
			ascii[j] = data[i];
		} else {
			ascii[j] = '.';
		}
	}

	if (j != 0) {
		printf("%s", buffer);
	}
}

// reads a binary file with the flag 'rb' on fopen
// return -1 if fail or the number of bytes read
int readbinaryfile(unsigned char * buffer, char * filename) {
	FILE * h;
	int i = 0;
	char tmp;
	
	h = fopen(filename, "rb");
	if (!h) {
		return -1;
	}
	
	while (!feof(h)) {
		fread(&tmp,1,1,h);
		buffer[i] = tmp;
		i++;
	}
	fclose(h);

	return i-1;
}

int getfilesize(char *filename) {
	FILE *h;
	int len;
	
	h = fopen(filename, "rb");
	if (!h) {
		return -1;
	}
	
	//Get file length
    fseek(h, 0, SEEK_END);
    len = ftell(h);
    fseek(h, 0, SEEK_SET);

	fclose(h);
	return len;
}
