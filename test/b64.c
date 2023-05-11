#include "../b64.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>



#define ERROR(a, ...) printf("%s:%d  " a "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__)

void b64t(char* test) {
	uint8_t buf[10000];
	uint64_t outlen = 0;
	
	base64_decode(test, strlen(test), buf, &outlen);
	
	for(uint64_t i = 0; i < outlen; i++) {
		printf("%c", buf[i]);
	}
	
	printf("\n");
}



int main(/*int argc, char* argv[]*/) {

	b64t("bGlnaHQgdw");
	b64t("bGlnaHQgd28");
	b64t("bGlnaHQgd29y");
	
	b64t("bGlnaHQgdw==");
	b64t("bGlnaHQgd28=");
	b64t("TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu");
	
	
	return 0;
}
