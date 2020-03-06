// Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <unistd.h>

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
	['_'] = "under",
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



typedef struct {
	int type; // 0 = char, 1 = charset, 2 = invchar, 3 = invcharset
	
	int c;
	char* cs_name;
	charset* cset;
	
	char* dest_state;
} state_case_info;

typedef struct state_info {
	char* name;
	VEC(state_case_info) c_cases;
	VEC(state_case_info) cs_cases;
	
	struct state_info* fail_to; 
	char* retry_as;
	
	char is_terminal;
} state_info;


state_info* new_state_info(char* name) {
	state_info* si = calloc(1, sizeof(*si));
	si->name  = strdup(name);
	return si;
}

state_case_info* add_case_char(state_info* si, int c, char* dest) {
	VEC_EACH(&si->c_cases, i, ci) {
		if(ci.type == 0 && ci.c == c) {
			if(0 != strcmp(dest, ci.dest_state)) {
				printf("case conflict: %s (%c)->%s / (%c)->%s\n", si->name, ci.c, ci.dest_state, c, dest);
				return NULL;
			}
			
			return &VEC_ITEM(&si->c_cases, i);
		}
	}
	
	VEC_INC(&si->c_cases);
	state_case_info* nci = &VEC_TAIL(&si->c_cases);
	nci->dest_state = strdup(dest);
	nci->type = 0;
	nci->c = c;
	
	return nci;
} 

state_case_info* add_case_cset(state_info* si, char* cset, char* dest) {
	VEC_EACH(&si->cs_cases, i, ci) {
		if(ci.type == 1 && 0 == strcmp(ci.cs_name, cset)) {
			if(0 != strcmp(dest, ci.dest_state)) {
				printf("case conflict: %s [%s]->%s / [%s]->%s\n", si->name, ci.cs_name, ci.dest_state, cset, dest);
				return NULL;
			}
			
			return &VEC_ITEM(&si->cs_cases, i);
		}
	}
	
	VEC_INC(&si->cs_cases);
	state_case_info* nci = &VEC_TAIL(&si->cs_cases);
	nci->dest_state = strdup(dest);
	nci->type = 1;
	nci->cs_name = strdup(cset);
	
	return nci;
} 



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
	VEC(state_info*) terminals;
	VEC(state_info*) internals;
	
	HashTable(state_info*) states;
	HashTable(charset) csets;
	
} tcontext;





#define MAX(a,b) ((a) > (b) ? (a) : (b))





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


typedef struct case_list {
	VEC(state_case_info) cases;
} case_list;

// expand a word into a set of transitions
// returns the final state
state_info* expand_word(char* word, state_info* base, case_list* extra, char* retry_as, tcontext* ctx) {
	
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
// 		printf("thisname: '%s'\n", this_name);
		
		state_info* s = induce_state(this_name, ctx);
// 			s->is_terminal = n->is_terminal;
		if(retry_as) s->retry_as = strdup(retry_as);
			
		// add the transitions
		
		state_case_info* ci = add_case_char(prev_st, c, this_name); 
		(void)ci;
		
		
		VEC_EACH(&extra->cases, i, cs) {
			add_case_cset(s, cs.cs_name, cs.dest_state);
		}
// 			
		
		
		// next
		prev_st = s;
		prev_name = this_name;
		(void)prev_name;
	}
	
	// the last state is a terminal state
	prev_st->is_terminal = 1;
	// TODO make the last statehave inverted logic for retry-as
	
	return prev_st;
}


static void print_state_switch(state_info* si) {
	printf("\ncase %s:\n", si->name);
	
			
	
	if(VEC_LEN(&si->c_cases)) {
		printf("\tswitch(c) {\n");
		char* slash = "\\";
		VEC_EACH(&si->c_cases, ckey, ci) {
			if(ci.type != 0) continue;
			int esc = (NULL != strchr("\\\t\n\r\v", ci.c)) ? 1 : 0; 
			printf("\t\tcase '%.*s%c': push_char_id(%s);\n", esc, slash, ci.c, ci.dest_state);
	// 			extract_table(si->words, tctx); // not used
	// 				printf("%s [%c] -> %s\n", key, ci.c, ci.dest_state);
			
		}
// 		printf("\tdefault:");
		if(VEC_LEN(&si->cs_cases)) {
			
			
		// 			}
		}
		
		printf("\t}\n");
		
	}
	
	if(VEC_LEN(&si->cs_cases)) {
		VEC_EACH(&si->cs_cases, ckey, ci) {
			if(0 == strcmp(si->retry_as, ci.cs_name) continue;
			printf("\tif(charset_has(%s, c)) { retry_as(%s); }\n", "charset_var", ci.cs_name);
		}
	}
	
	
	if(si->is_terminal)
		printf("\tgoto TOKEN_DONE;\n");
	else if(si->retry_as)
		printf("\tretry_as(%s);\n", si->retry_as);
	else 
		printf("\tgoto ERROR;\n");
}




static char* word_end(char* s, size_t* n) {
	char* end = strpbrk(s, " \n\t\r");
	if(!end) end = s + strlen(s);
	
	if(n) *n = end - s;
	
	return end;
}




static int state_sort_fn(void* a_, void* b_) {
	state_info** a = a_;
	state_info** b = b_;
	return strcmp((*a)->name, (*b)->name);
}
static int case_sort_fn(void* a_, void* b_) {
	state_case_info* a = a_;
	state_case_info* b = b_;
	return a->c - b->c;
}
static int case_cs_sort_fn(void* a_, void* b_) {
	state_case_info* a = a_;
	state_case_info* b = b_;
	return strcmp(a->cs_name, b->cs_name);
}



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
		
		
		char* cached_word = NULL;
		
		// word
// 		state_info* lst; // last state of the word
		if(*s == '{') {
			end = strpbrk(lines[i] + 1, " \n\t\r");
			wl = end - lines[i] - 1;
// 			printf(">%d %s\n", l, lines[i]+1);
			
			cached_word = strndup(s+1, wl);
			
// 			lst = expand_word(w, pst, tctx); 
			// put the word in the tree
// 			n = insert_word(pst->words, s + 1, wl);
			max_len = MAX(max_len, wl);
			
// 			free(w);
			
			// check for various metadata
			s = end;
			while(*s && *s == ' ') s++;
		}
		// single char transition
// 		else if(*s == '@') {
// 			s = word_end(++s, &wl);
// 		}
		char* retry_as = NULL;
		case_list extra;
		VEC_INIT(&extra.cases);
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
			
			// retry-as, the final failto
			if(*s == '|') {
				s++;
				
				end = word_end(s, &wl);
				retry_as = strndup(s, wl);
				
				s = end;
				
				continue;
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
				
				state_case_info* ci = add_case_char(pst, ec, fail_to); 
				(void)ci;
				free(fail_to);
				
				
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
				
				VEC_PUSH(&extra.cases, ((state_case_info){
					.type = 1,
					.cs_name = set_name,
					.dest_state = fail_to,
				}));
				
// 				state_case_info* ci = add_case_cset(pst, set_name, fail_to); 
// 				(void)ci;
// 				free(fail_to);
				
				s = end;
				continue;
			}
		
			printf("unknown s: '%c'\n", *s);
			s++;
		}
		
		
		if(cached_word) {
			expand_word(cached_word, pst, &extra, retry_as, tctx); 
		}
		
		VEC_EACH(&extra.cases, i, s) { free(s.cs_name); free(s.dest_state); }
		VEC_FREE(&extra.cases);
		if(retry_as) free(retry_as);
	}
	
	
// 	print_node(root);
	HT_LOOP(&tctx->states, key, state_info*, si) {
		VEC_SORT(&si->c_cases, case_sort_fn);
		VEC_SORT(&si->cs_cases, case_cs_sort_fn);

		if(si->is_terminal) {
			VEC_PUSH(&tctx->terminals, si);
		}
		else {
			VEC_PUSH(&tctx->internals, si);
		}
	}
	
	VEC_SORT(&tctx->terminals, state_sort_fn);
	VEC_SORT(&tctx->internals, state_sort_fn);
	
	
	
	if(print_switch) {
		
		printf("\n// terminals\n");
		VEC_EACH(&tctx->terminals, i, si) {
			print_state_switch(si);
		}
		
		printf("\n// internals\n");
		VEC_EACH(&tctx->internals, i, si) {
			print_state_switch(si);
		}
// 		HT_LOOP(&tctx->states, key, state_info*, si) {
// 			print_state_switch(si);
// 			
// 		}
		
	}
	
	
	
	if(print_enums) {
		
		
		printf("// terminals\n"); 
		VEC_EACH(&tctx->terminals, i, t) {
			printf("%s,\n", t->name);
		}
		
		printf("\n// internals\n"); 
		VEC_EACH(&tctx->internals, i, t) {
			printf("%s,\n", t->name);
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

