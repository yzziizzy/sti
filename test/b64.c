#include "../b64.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#define ERROR(a, ...) printf("%s:%d  " a "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

void b64t_decode(char* test) {
	uint8_t buf[10000];
	uint64_t outlen = 0;
	
	base64_decode(test, strlen(test), buf, &outlen);
	
	for(uint64_t i = 0; i < outlen; i++) {
		printf("%c", buf[i]);
	}
	
	printf("\n");
}


void b64t_encode(char* test) {
	char buf[10000];
	uint64_t outlen = 0;
	
	base64_encode(test, strlen(test), buf, &outlen);
	
	for(uint64_t i = 0; i < outlen; i++) {
		printf("%c", buf[i]);
	}
	
	printf("\n");
}


int main(/*int argc, char* argv[]*/) {

	b64t_decode("bGlnaHQgdw");
	b64t_decode("bGlnaHQgd28");
	b64t_decode("bGlnaHQgd29y");
	
	b64t_decode("bGlnaHQgdw==");
	b64t_decode("bGlnaHQgd28=");
	b64t_decode("TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu");
	
	b64t_encode("light w");
	b64t_encode("light wo");
	b64t_encode("light wor");
	b64t_encode("Many hands make light work.");
	
	return 0;
}
