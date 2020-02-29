// Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "fs.h" 
#include "vec.h" 
#include "string.h" 



char* exchg[] = {
	['!'] = "bang",
	['%'] = "pct",
	['['] = "lbracket",
	[']'] = "rbracket",
	['{'] = "lbrace",
	['}'] = "rbrace",
	['('] = "lparen",
	[')'] = "rparen",
	['^'] = "caret",
	['#'] = "pound",
	['@'] = "at",
	['~'] = "tilde",
	['&'] = "amp",
	['$'] = "dollar",
	['`'] = "grave",
	['\''] = "squote",
	['"'] = "dquote",
	[':'] = "colon",
	[';'] = "semi",
	['?'] = "quest",
	['.'] = "dot",
	[','] = "comma",
	['<'] = "lt",
	['>'] = "gt",
	['+'] = "plus",
	['='] = "eq",
	['_'] = "_",
	['-'] = "dash",
	['|'] = "pipe",
	['\\'] = "bslash",
	['/'] = "slash",
	['*'] = "star",
};




typedef struct node {
	int c;
	char is_terminal;
	
	VEC(struct node*) kids;
	
	char* type_name;
	char* fail_to;
	
} node;



static void print_node_(node* n, int lvl) {
	if(VEC_LEN(&n->kids) == 0) return;
	printf("%d: ", lvl);
	
	VEC_EACH(&n->kids, i, k) {
		printf("%c,", k->c);
	}
	
	if(n->is_terminal) printf("[t]");
	printf("\n");
	
	VEC_EACH(&n->kids, i, k) {
		print_node_(k, lvl+1);
	}
}

void print_node(node* n) {
	print_node_(n, 0);
}


static node* new_node(char c) {
	node* n = calloc(1, sizeof(*n));
	n->c = c;
	return n;
}


static node* get_kid(node* n, char c) {
	VEC_EACH(&n->kids, i, k) {
		if(k->c == c) return k;
	}
	return NULL;
}

static node* insert_word(node* root, char* word, int len) {
// 	if(word[0] == 0) return;
	
	node* k = get_kid(root, word[0]);
	if(!k) {
		k = new_node(word[0]);
		VEC_PUSH(&root->kids, k);
	}
	
	if(word[1] == 0 || len <= 0) {
		k->is_terminal = 1;
		return k;
	}
	else {
		return insert_word(k, word+1, len-1);
	}
}



typedef struct context {
	char* buffer;
	size_t alloc;
	size_t len;
	
	int level;
	
	VEC(char*) terminals;
	VEC(char*) internals;
	
} context;



static void extract_strings(node* n, context* ctx) {
// 	printf("entering: %d, %c\n", ctx->level, n->c);
	ctx->level++;
	
	if(n->c != 0) {
		ctx->buffer[ctx->len++] = n->c;
		ctx->buffer[ctx->len] = 0;
		
		if(n->is_terminal) {
			VEC_PUSH(&ctx->terminals, strdup(ctx->buffer));
		}
		else {
			VEC_PUSH(&ctx->internals, strdup(ctx->buffer));
		}
	}
	
	VEC_EACH(&n->kids, i, k) {
		extract_strings(k, ctx);
	}
	
	ctx->len--;
	ctx->level--;
}




#define MAX(a,b) ((a) > (b) ? (a) : (b))




// a test parser


// this is for the processing of the input stream overall
struct input_state {
	char* buffer;
	int length;
	int cursor;
	
	int tokenState;
	int tokenFinished;
};

// this is for the incremental lexing of each token, not the whole stream
struct lexer_state {
	int state;
	char* buffer;
	int blen;
	int balloc;
	
	int linenum;
	int charnum;
	
	size_t pastLeadingWS; // flag set to 1 upton first non-whitespace character on each line 
	char priorEscape; 
	char priorBackslash;
	
	int tokenState;
	int tokenFinished; // buffer should be consumed and cleaned at this point 
};









typedef struct tcontext {
	char* prefix;
	
	char* fbuffer;
	char* buffer;
	size_t alloc;
	size_t len;
	
	int level;
	
	size_t tlen;
	
	VEC(char*) terminals;
	VEC(char*) internals;
	
} tcontext;


/*
static void number_states(node* n, tcontext* ctx) {
	
	VEC_EACH(&n->kids, i, k) {
		k->state_num = ctx->tlen++;
	}
	
	VEC_EACH(&n->kids, i, k) {
		number_states(k, ctx);
	}
}
*/

static char* state_name(char* pre, char* r, int next) {
	char* b = malloc(4096);
	strcpy(b, pre);
	int len = strlen(pre);
	int last_was_an = 1;
	
	for(int i = 0; r[i] != 0; i++) {
		int rc = (int)r[i];
		if(isalnum(r[i])) {
			if(!last_was_an) {
				b[len++] = '_';
			}
			
			b[len++] = r[i];
			last_was_an = 1;
			continue;
		}
		
		if(exchg[rc]) {
// 			if(last_was_an) {
				b[len++] = '_';
// 			}
			
			strcpy(b + len, exchg[rc]);
			len += strlen(exchg[rc]);
			
			last_was_an = 0;
			continue;
		}
		
		printf("unhandleable char: %d\n", r[i]);
	}
	
	if(next == 0) {
		// nothin
	}
	else if(isalnum(next)) {
		if(!last_was_an) {
			b[len++] = '_';
		}
		
		b[len++] = next;
	}
	else if(exchg[next]) {
		b[len++] = '_';
		
		strcpy(b + len, exchg[next]);
		len += strlen(exchg[next]);
	}
	else {
		printf("unhandleable char: %d\n", next);
	}
	
	return b;
}


#define case_c(c) case 'c':  

static void extract_table(node* n, tcontext* ctx) {
// 	printf("entering: %d, %c\n", ctx->level, n->c);
	ctx->level++;
	
	
	
	if(n->c != 0) {
		
		ctx->buffer[ctx->len++] = n->c;
		ctx->buffer[ctx->len] = 0;
		
// 		char* word = strdup(ctx->buffer);
		
		char* sname = state_name(ctx->prefix, ctx->buffer, 0); 
		
// 		printf("name: %s\n", sname);
	
		printf("case %s: ", sname);
		
		
		if(n->is_terminal) {
			printf("goto TOKEN_DONE;\n");
			VEC_PUSH(&ctx->terminals, sname);
		}
		else {
			VEC_PUSH(&ctx->internals, sname);
		}
		
		
		if(VEC_LEN(&n->kids)) {
			printf("switch(n) {\n");
			VEC_EACH(&n->kids, i, k) {
				char* nn = state_name(ctx->prefix, ctx->buffer, k->c);
				printf("\tcase '%c': push_char_id(%s);\n", k->c, nn);
				free(nn);
			}
			printf("\tdefault: goto ERROR;\n");
			printf("\t}\n");
			
		}
		
	}
	VEC_EACH(&n->kids, i, k) {
		extract_table(k, ctx);
	}
	
	ctx->len--;
	ctx->level--;
}

static char* word_end(char* s, size_t* n) {
	char* end = strpbrk(s, " \n\t\r");
	if(!end) end = s + strlen(s);
	
	if(n) *n = end - s;
	
	return end;
}


int main(int argc, char* argv[]) {
	size_t flen;
	(void)argc;
	
	size_t max_len = 0;
	
	char* src = readWholeFile(argv[1], &flen);
	char** lines = strsplit_inplace(src, '\n', NULL);
	
	node* root = new_node(0);
	
	for(int i = 0; lines[i]; i++) {
		int c = lines[i][0];
		char* s, *end;
		size_t wl;
		
		if(c == 0) continue; // empty line
		if(c == '#') continue; // comments
		
		if(c == '>') { // word
			end = strpbrk(lines[i] + 1, " \n\t\r");
			wl = end - lines[i] - 2;
// 			printf(">%d %s\n", l, lines[i]+1);
			
			// put the word in the tree
			node* n = insert_word(root, lines[i] + 1, wl);
			max_len = MAX(max_len, strlen(lines[i]));
			
			// add its metadata
			s = end;
			while(*s && *s == ' ') s++;
			end = word_end(s, &wl);
			
			n->type_name = strndup(s, wl);
			
			
			s = end;
			while(*s && *s == ' ') s++;
			end = word_end(s, &wl);
			
			n->fail_to = strndup(s, wl); 
// 			printf("%s\n", n->type_name);
			
			continue;
		}
		
		if(c == '[') { // character set
			s = lines[i] + 1;
			end = word_end(s, &wl);
			
			s = end;
			while(*s && *s == ' ') s++;
			end = word_end(s, &wl);
			
			VEC(char) chars;
			VEC_INIT(&chars);
			while(*s && *s == ' ') s++;
			while(*s && *s != ' ') {
				if(s[0] == '\\') { // escape
					VEC_PUSH(&chars, s[1]);
					s++;
				}
				else if(s[1] == '-') { // range
					char low = s[0]; 
					char high = s[2];
					
					for(char c = low; c <= high; c++) {
						VEC_PUSH(&chars, c);
					}
					
					s += 2;
				}
				else {
					VEC_PUSH(&chars, s[0]);
				}
				
				s++;
			}
			
			printf("[%.*s\n", (int)chars.len, chars.data);
		}
	}
	
// 	print_node(root);
	
	context* ctx = calloc(1, sizeof(*ctx));
	ctx->alloc = max_len + 1;
	ctx->buffer = malloc(ctx->alloc);
	
	extract_strings(root, ctx);
	
	
	VEC_EACH(&ctx->terminals, i, t) {
		printf("t: %ld: '%s'\n", i, t);
	}
	VEC_EACH(&ctx->internals, i, t) {
		printf("i: %ld: '%s'\n", i, t);
	}
	
	
	
	// build a table
	tcontext* tctx = calloc(1, sizeof(*tctx));
	tctx->alloc = max_len + 1;
	tctx->fbuffer = malloc(tctx->alloc);
	tctx->prefix = "DST_";
	strcpy(tctx->fbuffer, tctx->prefix);
	tctx->buffer = tctx->fbuffer + strlen(tctx->prefix);
	
// 	number_states(root, tctx);
	
	extract_table(root, tctx);
	
	
	
	
	
	
	// push char, accept token, change state, retry with new state
	
	return 0;
}

