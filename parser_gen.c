// Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "fs.h" 
#include "vec.h" 
#include "string.h" 
#include "hash.h" 



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


typedef struct charset {
	char* name;
	char* raw;
	
	char* table;
	int minval;
	int maxval;
	int len;
} charset;

static void build_charset(charset* cs) {
	cs->minval = 9999999;
	cs->maxval = -9999999;
	for(int i = 0; cs->raw[i] != 0; i++) {
		if(cs->raw[i] < cs->minval) cs->minval = cs->raw[i];
		if(cs->raw[i] > cs->maxval) cs->maxval = cs->raw[i];
	}
	
	cs->len = cs->maxval - cs->minval;
	cs->table = calloc(1, sizeof(*cs->table) * cs->len);
	
	for(int i = 0; cs->raw[i] != 0; i++) {
		cs->table[(int)cs->raw[i] - cs->minval] = 1;
	}
}

int charset_has(charset* cs, int c) {
	int i = c - cs->minval;
	if(i > cs->maxval || i < 0) return 0;
	return cs->table[i];
}


typedef struct node {
	int c;
	char is_terminal;
	
	VEC(struct node*) kids;
	
	char* type_name;
	
	char* fail_charset;
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

node* insert_word(node* root, char* word, int len) {
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


typedef struct {
	int type; // 0 = char, 1 = charset, 2 = invchar, 3 = invcharset
	
	int c;
	char* cs_name;
	charset* cset;
	
	char* dest_state;
} state_case_info;

typedef struct {
	char* name;
	VEC(state_case_info) cases;
	
	node* words;
	
	char is_terminal;
} state_info;


state_info* new_state_info(char* name) {
	state_info* si = calloc(1, sizeof(*si));
	si->name  = strdup(name);
	si->words = new_node(0);
	return si;
}


typedef struct context {
	char* buffer;
	size_t alloc;
	size_t len;
	
	int level;
	
	VEC(char*) terminals;
	VEC(char*) internals;
	
} context;



typedef struct tcontext {
	char* prefix;
	
	// tree-walking buffer
	char* fbuffer;
	char* buffer;
	size_t alloc;
	size_t len;
	
	int level;
	
	size_t tlen;
	
	// categorized state names
	VEC(char*) terminals;
	VEC(char*) internals;
	
	HashTable(state_info*) states;
	HashTable(charset) csets;
	
} tcontext;


void extract_strings(node* n, tcontext* ctx) {
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
	b[len++] = '_';
	b[len++] = '_';
	
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
	
	b[len] = 0;
	
	return b;
}


state_info* induce_state(char* name, tcontext* ctx) {
	state_info* s;
	
	if(HT_get(&ctx->states, name, (void*)&s)) {
		s = new_state_info(name);
		HT_set(&ctx->states, name, s);
	}
	
	return s;
}

/*
char* state_name_inc(char* prefix, char* r) {
	char* b = malloc(256);
	int alloc = 256;
	int len;
	int last_was_an = 1;
	
	len = strlen(prefix);
	strcpy(b, prefix);
	
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
	
	b[len] = 0;
	
	return b;
}
*/

// expand a word into a set of transitions
// returns the final state
state_info* expand_word(char* word, state_info* base, tcontext* ctx) {
	
	size_t blen = 0;
	size_t balloc = 256;
	char* buffer = malloc(balloc * sizeof(*buffer));
	
	char* prev_name = base->name;
	state_info* prev_st = base;
	
	
	for(int i = 0; word[i] != 0; i++) {
		int c = word[i];
		if(c == '\\') {
			i++;
			c = word[i];
			if(c == 't') c = '\t';
			else if(c == 'r') c = '\r';
			else if(c == 'n') c = '\n';
			else if(c == 'v') c = '\v';
			else if(c == '\\') c = '\\';
			else if(c == 0) {
				// malformed word
				break;
			}
		}
		
		// collect up new name
		buffer[blen++] = c;
		buffer[blen] = 0;
		char* this_name = state_name(base->name, buffer, 0);
		printf("thisname: '%s'\n", this_name);
		
		state_info* s = induce_state(this_name, ctx);
// 			s->is_terminal = n->is_terminal;
			
		// add the transitions
		VEC_INC(&prev_st->cases);
		state_case_info* ci = &VEC_TAIL(&prev_st->cases); 
			
		ci->type = 0; // single char
		ci->c = c;
		ci->dest_state = this_name;
		
		// next
		prev_st = s;
		prev_name = this_name;
		(void)prev_name;
	}
	
	// the last state is a terminal state
	prev_st->is_terminal = 1;
	
	return prev_st;
}


// expand the prefix tree into a set of transitions
void expand_words(node* n, state_info* pst, tcontext* ctx) {
	ctx->level++;
	
	// root node
	if(n->c == 0) {
		
		VEC_EACH(&n->kids, i, k) {
			char* nn = state_name(ctx->prefix, ctx->buffer, k->c);
// 			printf("\tcase '%c': push_char_id(%s);\n", k->c, nn);
			state_info* s = induce_state(nn, ctx);
			s->is_terminal = n->is_terminal;
			
			VEC_INC(&pst->cases);
			state_case_info* ci = &VEC_TAIL(&pst->cases); 
				
			ci->type = 0; // single char
			ci->c = k->c;
			ci->dest_state = nn;
			
			expand_words(k, s, ctx);
		}
		
// 		printf("\tdefault: goto ERROR;\n");
// 		printf("\t}\n");
	} 
	// internal nodes
	else {
		ctx->buffer[ctx->len++] = n->c;
		ctx->buffer[ctx->len] = 0;
		
// 		char* psname = state_name(ctx->prefix, ctx->buffer, 0); 
// 		state_info* s = induce_state(sname, tctx);
// 		s->is_terminal = n->is_terminal;
		
		VEC_EACH(&n->kids, i, k) {
			char* nn = state_name(ctx->prefix, ctx->buffer, k->c);
// 			printf("\tcase '%c': push_char_id(%s);\n", k->c, nn);
			state_info* s = induce_state(nn, ctx);
			s->is_terminal = n->is_terminal;
			
			VEC_INC(&pst->cases);
			state_case_info* ci = &VEC_TAIL(&pst->cases); 
				
			ci->type = 0; // single char
			ci->c = k->c;
			ci->dest_state = nn;
			
			expand_words(k, s, ctx);
		}
		
	}
	
	
	ctx->len--;
	ctx->level--;
	
	
}


void extract_table(node* n, tcontext* ctx) {
// 	printf("entering: %d, %c\n", ctx->level, n->c);
	ctx->level++;
	
	
	
	if(n->c == 0) {
		
		printf("case LST_NULL: ");
		printf("switch(c) {\n");
		VEC_EACH(&n->kids, i, k) {
			char* nn = state_name(ctx->prefix, ctx->buffer, k->c);
			printf("\tcase '%c': push_char_id(%s);\n", k->c, nn);
			free(nn);
		}
		
		printf("\tdefault: goto ERROR;\n");
		printf("\t}\n");
	} 
	else {
		ctx->buffer[ctx->len++] = n->c;
		ctx->buffer[ctx->len] = 0;
		
// 		char* word = strdup(ctx->buffer);
		
		char* sname = state_name(ctx->prefix, ctx->buffer, 0); 
		
// 		printf("name: %s\n", sname);
	
		printf("case %s: ", sname);
		
		
		if(n->is_terminal) {
			if(n->fail_to) {
				printf("\n");
				printf("\tif(charset_has(%s, c)) { retry_as(%s); }\n", "charset_var", n->fail_to);
				printf("\tgoto TOKEN_DONE;\n");
			}
			else {
				printf("goto TOKEN_DONE;\n");
			}
			VEC_PUSH(&ctx->terminals, sname);
		}
		else {
			VEC_PUSH(&ctx->internals, sname);
		}
		
		
		if(VEC_LEN(&n->kids)) {
			printf("switch(c) {\n");
			VEC_EACH(&n->kids, i, k) {
				char* nn = state_name(ctx->prefix, ctx->buffer, k->c);
				printf("\tcase '%c': push_char_id(%s);\n", k->c, nn);
				free(nn);
			}
			
			printf("\tdefault:");
// 			if(n->fail_charset) {
// 				printf("\n");
// 				printf("if(charset_has(c)");
// 				
// 			}
// 			else {
			if(n->is_terminal)
				printf(" goto TOKEN_DONE;\n");
			else 
				printf(" goto ERROR;\n");
// 			}
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


#include <unistd.h>



int main(int argc, char* argv[]) {
	char ac;
	char print_enums = 0;
	char print_switch = 0;
	char print_csets = 0;
	char* enum_pattern = NULL;
	char* terminal_pattern = NULL;
	char* fname = NULL;
	char* prefix = "LST__";
	
	while((ac = getopt(argc, argv, "cesE:T:")) != -1) {
		switch(ac) {
			case 'c': print_csets = 1; break;
			case 'e': print_enums = 1; break;
			case 'E': 
				print_enums = 1; 
				enum_pattern = optarg;
				break;
			case 'T': 
				print_enums = 1; 
				terminal_pattern = optarg;
				break;
			case 's': print_switch = 1; break;
			/* TODO: 
			combine like states into one fall-through case.
			default start state name
			
			*/
		}
	}
	
	if(optind < argc) {
		fname = argv[optind];
	}
	
	(void)terminal_pattern;
	(void)enum_pattern;
	
	size_t flen;
	size_t max_len = 0;
	
	char* src = readWholeFile(fname, &flen);
	char** lines = strsplit_inplace(src, '\n', NULL);
	
// 	node* root = new_node(0);
	
// 	HashTable(state_info) states;
// 	HT_init(&states, 1024);
	
// 	HashTable(charset) csets;
// 	HT_init(&csets, 64);
	

	tcontext* tctx = calloc(1, sizeof(*tctx));
	tctx->alloc = max_len + 1;
	tctx->fbuffer = malloc(tctx->alloc);
	tctx->prefix = prefix;
	strcpy(tctx->fbuffer, tctx->prefix);
	tctx->buffer = tctx->fbuffer + strlen(tctx->prefix);
	
	HT_init(&tctx->states, 1024);
	HT_init(&tctx->csets, 64);
	
	
	
	for(int i = 0; lines[i]; i++) {
		int c = lines[i][0];
		char* s, *end;
		size_t wl;
		char* state_prefix = strdup("LST_NULL"); // TODO unhardcode
		
		s = lines[i];
		
// 		printf("i: %d, '%c/%d'\n", i, *s, *s);
		if(c == 0) continue; // empty line
		if(c == '#') continue; // comments
		
		// character set declaration
		if(c == '[') {
			s = lines[i] + 1;
			end = word_end(s, &wl);
			
			// the name
			char* sname = strndup(s, wl);
			
			s = end;
			while(*s && *s == ' ') s++;
			end = word_end(s, &wl);
			
			// char set itself
			VEC(char) chars;
			VEC_INIT(&chars);
			while(*s && *s == ' ') s++;
			while(*s && *s != ' ') {
				if(s[0] == '\\') { // escape
					int ec = s[1];
					if(ec == 't') ec = '\t';
					else if(ec == 'r') ec = '\r';
					else if(ec == 'n') ec = '\n';
					else if(ec == 'v') ec = '\v';
					else if(ec == '\\') ec = '\\';
					VEC_PUSH(&chars, ec);
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
				else { // normal char
					VEC_PUSH(&chars, s[0]);
				}
				
				s++;
			}
			
			VEC_PUSH(&chars, 0);
			
			charset* cs = calloc(1, sizeof(*cs));
			cs->raw = strdup(chars.data);
			cs->name = sname;
			build_charset(cs);
			
			if(HT_set(&tctx->csets, sname, cs)) {
				printf("ht fail: '%s'\n", sname);
			}
			
			printf("%s: '%s'\n", sname, chars.data);
			VEC_FREE(&chars);
			
			continue;
		}

		
		// state prefix
		if(c == ':') {
			end = word_end(++s, &wl);
			state_prefix = strndup(s, wl);
			s = end;
			while(*s && *s == ' ') s++;
		}
		else {
			state_prefix = strdup("LST_NULL");
		}
		
		state_info* pst;
		if(HT_get(&tctx->states, state_prefix, (void*)&pst)) {
			pst = new_state_info(state_prefix);
			HT_set(&tctx->states, state_prefix, pst);
		}
		
		
		// word
		state_info* lst; // last state of the word
		if(*s == '{') {
			end = strpbrk(lines[i] + 1, " \n\t\r");
			wl = end - lines[i] - 2;
// 			printf(">%d %s\n", l, lines[i]+1);
			
			char* w = strndup(s+1, wl);
			
			lst = expand_word(w, pst, tctx); 
			// put the word in the tree
// 			n = insert_word(pst->words, s + 1, wl);
			max_len = MAX(max_len, wl);
			
			free(w);
			
			// check for various metadata
			s = end;
			while(*s && *s == ' ') s++;
		}
		// single char transition
// 		else if(*s == '@') {
// 			s = word_end(++s, &wl);
// 		}
		
		
		while(*s && *s != '\r' && *s != '\n') {
			while(*s && *s == ' ') s++;
			if(!s) break;
			
			if(*s == ':') { // token type
				end = word_end(++s, &wl);
// 				n->type_name = strndup(s, wl);
				s = end;
				continue;
			}
			
			
			// go-to
			if(*s == '>') {
				// TODO this one is wrong
				break;
			}
			
			// char fail-to
			if(*s == '@') {
				s++;
				int ec = *s;
				if(ec == '\\') {
					ec = *(s++);
					if(ec == 't') ec = '\t';
					else if(ec == 'r') ec = '\r';
					else if(ec == 'n') ec = '\n';
					else if(ec == 'v') ec = '\v';
					// TODO: hex/unicode exscape
					// else the literal char
				}
				
				// the target state
				s = end + 1;
				end = word_end(s, &wl);
				char* fail_to = strndup(s, wl);
				
				VEC_INC(&pst->cases);
				state_case_info* ci = &VEC_TAIL(&pst->cases); 
				
				ci->type = 0; // single char
				ci->c = ec;
				ci->dest_state = fail_to;
				
				s = end;
				continue;
			}
			
			// charset fail-to
			if(*s == '+') {
				s++;
				end = strchr(s, '>');
				
				char* set_name = strndup(s, end - s);
				
				
				s = end + 1;
				end = word_end(s, &wl);
				
				char* fail_to = strndup(s, wl);
				
				// TODO: look up lazily later
// 				if(HT_get(&csets, set_name, (void*)&n->fail_charset)) {
// 					printf("failed to get charset: %p '%s'\n", n->fail_charset, set_name);
// 				}
				// TODO: state transition info
				
				VEC_INC(&pst->cases);
				state_case_info* ci = &VEC_TAIL(&pst->cases); 
				
				ci->type = 1; // charset
				ci->cs_name = set_name;
				ci->dest_state = fail_to;
				
				s = end;
				continue;
			}
		
			printf("unknown s: '%c'\n", *s);
			s++;
		}
		
		(void)lst;
	}
	
	
// 	print_node(root);
	
	
	if(print_switch) {
		
		
		HT_LOOP(&tctx->states, key, state_info*, si) {
// 			extract_table(si->words, tctx); // not used
			
			// reset context fields
			tctx->len = 0;
			tctx->level = 0;
		}
		
	}
	
	
	
	if(print_enums) {
		
		HT_LOOP(&tctx->states, key, state_info*, si) {
			extract_strings(si->words, tctx);
			
			// reset context fields
			tctx->len = 0;
			tctx->level = 0;
		}
		
		printf("// terminals\n"); 
		VEC_EACH(&tctx->terminals, i, t) {
			char* sname = state_name(prefix, t, 0); 
			printf("%s,\n", sname);
			free(sname);
		}
		
		printf("\n// internals\n"); 
		VEC_EACH(&tctx->internals, i, t) {
			char* sname = state_name(prefix, t, 0); 
			printf("%s,\n", sname);
			free(sname);
		}
		
	}
	
	
	
	if(print_csets) {
		
		HT_LOOP(&tctx->csets, key, charset*, cs) {
			printf("char cset_%s[] = {", cs->name);
			for(int i = 0; i < cs->minval; i++) printf("0,"); 
			for(int i = cs->minval; i <= cs->maxval; i++) printf("%d,", !!cs->table[i]); 
			printf("0};\n");
			printf("int cset_%s_len = %d;\n", cs->name, cs->maxval);
		}
		
	}
	
	
	
	return 0;
}

