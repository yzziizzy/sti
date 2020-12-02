// Public Domain

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include <unistd.h>

#include "../fs.h" 
#include "../vec.h" 
#include "../string.h" 
#include "../hash.h" 


int has_case_conflict = 0;


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
	int action; // > each char, ^ discard char
	int invert;
	
	int c;
	char* cs_name;
	charset* cset;
	
	char* dest_state;
} state_case_info;

typedef struct strpair {
	char* key;
	char* val;
} strpair;

typedef struct state_info {
	char* name;
	VEC(state_case_info) c_cases;
	VEC(state_case_info) cs_cases;
	
	struct state_info* fail_to; 
	char* retry_as;
	char* retry_as_cs_name;
	
	char is_terminal;
	VEC(strpair) terminal_data;
} state_info;


state_info* new_state_info(char* name) {
	state_info* si = calloc(1, sizeof(*si));
	si->name  = strdup(name);
	return si;
}

state_case_info* add_case_char(state_info* si, int c, int invert, char* dest) {
	VEC_EACH(&si->c_cases, i, ci) {
		if(ci.type == 0 && ci.c == c && ci.invert == invert) {
			if(0 != strcmp(dest, ci.dest_state)) {
				printf("case conflict: %s (%c)->%s / (%c)->%s\n", si->name, ci.c, ci.dest_state, c, dest);
				has_case_conflict = 1;
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
	nci->action = 0; // consume character
	nci->invert = invert; 
	
	return nci;
} 

state_case_info* add_case_cset(state_info* si, char* cset, char* dest, int action, int invert) {
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
	nci->action = action;
	nci->invert = invert;
	
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
	
	HT(state_info*) states;
	HT(charset*) csets;
	
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
		HT_set(&ctx->states, strdup(name), s); // BUG fix needing to strdup the name
	}
	
	return s;
}


typedef struct case_list {
	VEC(state_case_info) cases;
} case_list;

// expand a word into a set of transitions
// returns the final state
state_info* expand_word(
	char* word, 
	state_info* base, 
	case_list* extra, 
	char* retry_as, 
	char* retry_as_cs_name, 
	tcontext* ctx) {
	
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
		if(retry_as_cs_name) s->retry_as_cs_name = strdup(retry_as_cs_name);
			
		// add the transitions
		
		state_case_info* ci = add_case_char(prev_st, c, 0, this_name); 
		(void)ci;
		
		
		VEC_EACH(&extra->cases, i, cs) {
			add_case_cset(s, cs.cs_name, cs.dest_state, cs.action, cs.invert);
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

// buff must be at least as long as it needs to be
static void pretty_print_char(char* buf, int c) {
	
#define ESC_CHAR(x) \
	buf[0] = '\''; \
	buf[1] = '\\'; \
	buf[2] = x; \
	buf[3] = '\''; \
	buf[4] = 0; \
	break;
	
	switch(c) {
		case '\t': ESC_CHAR('t');
		case '\r': ESC_CHAR('r');
		case '\n': ESC_CHAR('n');
		case '\v': ESC_CHAR('v');
		case '\'': ESC_CHAR('\'');
		case '\\': ESC_CHAR('\\');
			
		default:
			if(isgraph(c)) {
				buf[0] = '\'';
				buf[1] = c;
				buf[2] = '\'';
				buf[3] = 0;
			}
			else {
				sprintf(buf, "0x%x", c);
			}
	}
}

static void print_state_switch(state_info* si) {
	char buf[16];
	
	printf("\ncase %s:\n", si->name);
	
			
	
	if(VEC_LEN(&si->c_cases)) {
		printf("\tswitch(c) {\n");
		VEC_EACH(&si->c_cases, ckey, ci) {
			if(ci.invert) continue;
			
			pretty_print_char(buf, ci.c);
			char* action = "push_char_id";
			if(ci.action == 1) action = "push_char_done";
			else if(ci.action == 2) action = "done_zero_move";
			printf("\t\tcase %s: %s(%s);\n", buf, action, ci.dest_state);
		}
		
		printf("\t}\n");
		
		VEC_EACH(&si->c_cases, ckey, ci) {
			if(!ci.invert) continue;

			pretty_print_char(buf, ci.c);
			char* action = "push_char_id";
			if(ci.action == 1) action = "push_char_done";
			else if(ci.action == 2) action = "done_zero_move";
			printf("\tif(c != %s) { %s(%s); }\n", buf, action, ci.dest_state);
		}
	}
	
	if(VEC_LEN(&si->cs_cases)) {
		VEC_EACH(&si->cs_cases, ckey, ci) {
			if(si->retry_as && 0 == strcmp(si->retry_as, ci.cs_name)) continue;
			char* action = ci.action == '^' ? "discard_char_id" : "push_char_id";
			char* inv = ci.invert ? "!" : "";
			printf("\tif(%scharset_has(cset_%s, c)) { %s(%s); }\n", inv, ci.cs_name, action, ci.dest_state);
		}
	}
	
	
	if(si->fail_to) {
		printf("push_char_id(%s);\n", si->fail_to);
	}
	else if(si->is_terminal) {
		if(si->retry_as) printf("\tif(charset_has(cset_%s, c)) { retry_as(%s); }\n", si->retry_as_cs_name, si->retry_as);
		printf("\tdone_zero_move(%s);\n", si->name);

	}
	else if(si->retry_as)
		printf("\tretry_as(%s);\n", si->retry_as);
	else 
		printf("\tgoto ERROR;\n");
}




static char* word_end(char* s, size_t* n) {
	// TODO: suport escaped spaces
	
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
	char print_data = 0;
	char print_data_defs = 0;
	char print_unique_data = 0;
	char print_triplet_data = 0;
	char print_enums = 0;
	char print_enum_names = 0;
	char print_guards = 0;
	char print_macros = 0;
	char print_switch = 0;
	char print_csets = 0;
	char* enum_pattern = NULL;
	char* terminal_pattern = NULL;
	char* fname = NULL;
	char* prefix = "LST__";
	char* guard_prefix = "PARSER_INCLUDE";
	char* data_def_prefix = "STATE_DATA_DEF_";
	
	while((ac = getopt(argc, argv, "cdD:eE:fF:gG:mnsT:uv")) != -1) {
		switch(ac) {
			case 'c': print_csets = 1; break;
			case 'd': print_data = 1; break;
			case 'D': 
				print_data = 1; 
				data_def_prefix = optarg;
				break;
			case 'e': print_enums = 1; break;
			case 'f': print_data_defs = 1; break;
			case 'F': 
				print_data_defs = 1; 
				data_def_prefix = optarg;
				break;
			case 'g': print_guards = 1; break;
			case 'G': 
				print_guards = 1;
				guard_prefix = optarg;
				break;
			case 'E': 
				print_enums = 1; 
				enum_pattern = optarg;
				break;
			case 'm': print_macros = 1; break; 
			case 'n': print_enum_names = 1; break;
			case 's': print_switch = 1; break;
			case 'T': 
				print_enums = 1; 
				terminal_pattern = optarg;
				break;
			case 'u': print_unique_data = 1; break;
			case 'v': print_triplet_data = 1; break;
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
		char* state_prefix = NULL;//strdup("LST_NULL"); // TODO unhardcode
		
		s = lines[i];
		
// 		printf("i: %d, '%c/%d'\n", i, *s, *s);
		if(c == 0) continue; // empty line
		if(c == '#') continue; // comments
		
		VEC(strpair) terminal_data;
		VEC_INIT(&terminal_data);
		
		// mark state as terminal
		if(c == '&') {
			s = lines[i] + 1;
			end = word_end(s, &wl);
			
			// the name
			char* sname = strndup(s, wl);
			
			state_info* si = induce_state(sname, tctx);
			si->is_terminal = 1;
			
			s = end;
			
			// save any terminal data
			while(*s) {
				while(*s && *s == ' ') s++;
				if(!*s || *s == '\r' || *s == '\n') break;
				
				if(*s == ':') { // token type
					
					char* td_key = NULL;
					char* td_val = NULL;
					
					s++;
					end = strpbrk(s, "= \n\t\r");
					if(!end) end = s + strlen(s);
					wl = end - s;
					td_val = strndup(s, wl);
					
					if(*end == '=') { // key pair
						td_key = td_val;
						s = end;
						
						end = word_end(++s, &wl);
						td_val = strndup(s, wl);
					}
					
					VEC_PUSH(&si->terminal_data, ((strpair){td_key, td_val}));
					
					s = end;
				}
			}
			
			
			free(sname);
			
			continue;
		}
		
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
			while(*s && !isspace(*s)) {
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
			
// 			printf("%s: '%s'\n", sname, chars.data);
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
		
		state_info* pst = induce_state(state_prefix, tctx);
// 		if(HT_get(&tctx->states, state_prefix, (void*)&pst)) {
// 			pst = new_state_info(state_prefix);
// 			HT_set(&tctx->states, state_prefix, pst);
// 		}
		
		
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
		char* retry_as_cs_name = NULL;
		
		
		case_list extra;
		VEC_INIT(&extra.cases);
		while(*s) {
			int invert = 0;
			while(*s && *s == ' ') s++;
			if(!*s || *s == '\r' || *s == '\n') break;
			
			if(*s == '!') {
				s++;
				invert = 1;
				if(!*s) break;
			}
			
			if(*s == ':') { // token type
				char* td_key = NULL;
				char* td_val = NULL;
				
				s++;
				end = strpbrk(s, "= \n\t\r");
				if(!end) end = s + strlen(s);
				wl = end - s;
				td_val = strndup(s, wl);
				
				if(*end == '=') { // key pair
					td_key = td_val;
					s = end;
					
					end = word_end(++s, &wl);
					td_val = strndup(s, wl);
				}
				
				VEC_PUSH(&terminal_data, ((strpair){td_key, td_val}));
				
				s = end;
				continue;
			}
			
			
			// go-to
			if(*s == '>') {
				s++;
				end = word_end(s, &wl);
				char* fail_to = strndup(s, wl);
				pst->fail_to = fail_to;
				
				break;
			}
			
			// retry-as, the final failto
			if(*s == '|') {
				s++;
				end = strchr(s, '>');
				
				char* set_name = strndup(s, end - s);
				
				
				s = end + 1;
				end = word_end(s, &wl);
				
				
				end = word_end(s, &wl);
				retry_as = strndup(s, wl);
				retry_as_cs_name = set_name;
				
				s = end;
				
				continue;
			}
			
			// char fail-to
			if(*s == '@') {
				s++;
				int ec = *s;
				if(ec == '\\') {
					ec = *++s;
					if(ec == 't') ec = '\t';
					else if(ec == 'r') ec = '\r';
					else if(ec == 'n') ec = '\n';
					else if(ec == 'v') ec = '\v';
					// TODO: hex/unicode exscape
					// else the literal char
				}
				s++;
				
				int type = *s++;
				
				// the target state
				end = word_end(s, &wl);
				char* fail_to = strndup(s, wl);
				
				state_case_info* ci = add_case_char(pst, ec, invert, fail_to);
				if(type == '>') ci->action = 0;
				else if(type == '=') ci->action = 1;
				else if(type == '~') ci->action = 2;
				
				
				free(fail_to);
				
				s = end;
				continue;
			}
			
			// charset fail-to
			if(*s == '+') {
				s++;
				end = strpbrk(s, ">^~");
				
				char* set_name = strndup(s, end - s);
				
				
				int type = *end;
				
				s = end + 1;
				end = word_end(s, &wl);
				
				char* fail_to = strndup(s, wl);
				
				// TODO: look up lazily later
// 				if(HT_get(&csets, set_name, (void*)&n->fail_charset)) {
// 					printf("failed to get charset: %p '%s'\n", n->fail_charset, set_name);
// 				}
				// TODO: state transition info
				
				int action = 0;
				if(type == '>') action = 0;
				else if(type == '=') action = 1;
				else if(type == '~') action = 2;
				
				
				VEC_PUSH(&extra.cases, ((state_case_info){
					.type = 1,
					.cs_name = set_name,
					.dest_state = fail_to,
					.action = action,
					.invert = invert,
				}));
				
				(void)fail_to;
				(void)set_name;
				
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
			state_info* final = expand_word(cached_word, pst, &extra, retry_as, retry_as_cs_name, tctx); 
			VEC_CAT(&final->terminal_data, &terminal_data);
		}
		else {
			VEC_CAT(&pst->terminal_data, &terminal_data);
			VEC_EACH(&extra.cases, i, ci) {
				add_case_cset(pst, ci.cs_name, ci.dest_state, ci.action, ci.invert);
			}
		}
		
		cached_word = NULL;
		
		VEC_EACH(&extra.cases, i, s) { free(s.cs_name); free(s.dest_state); }
		VEC_FREE(&extra.cases);
		VEC_FREE(&terminal_data);
		if(retry_as) free(retry_as);
		if(retry_as_cs_name) free(retry_as_cs_name);
	}
	
	
// 	print_node(root);
	HT_EACH(&tctx->states, key, state_info*, si) {
		VEC_SORT(&si->c_cases, case_sort_fn);
		VEC_SORT(&si->cs_cases, case_cs_sort_fn);

		if(si->is_terminal) {
// 			printf("t:%s %p, %s, %s\n", key, (void*)si, si->name, si->retry_as);
			VEC_PUSH(&tctx->terminals, si);
		}
		else {
// 			printf("i:%s %p, %s\n", key, (void*)si, si->name);
			VEC_PUSH(&tctx->internals, si);
		}
	}
	
	VEC_SORT(&tctx->terminals, state_sort_fn);
	VEC_SORT(&tctx->internals, state_sort_fn);
	
	if(has_case_conflict) exit(1);
	
	
	if(print_macros) {
		if(print_guards) printf("#ifdef %s_MACROS\n", guard_prefix);
		printf(
			"#define push_char_id(_state) \\\n"
			"do { \\\n"
			"	st->state = _state; \\\n"
			"	goto PUSH_CHAR_RET; \\\n"
			"} while(0)\n"
			"\n"
			"\n"
			"#define discard_char_id(_state) \\\n"
			"do { \\\n"
			"	st->state = _state; \\\n"
			"	return 1; \\\n"
			"} while(0)\n"
			"\n"
			"\n"
			"#define retry_as(_state) \\\n"
			"do { \\\n"
			"	st->state = _state; \\\n"
			"	goto RETRY; \\\n"
			"} while(0);\n"
			"\n"
			"#define done_zero_move(_state) \\\n"
			"do { \\\n"
			"	st->state = _state; \\\n"
			"	goto TOKEN_DONE; \\\n"
			"} while(0);\n"
			"\n"
			"#define push_char_done(_state) \\\n"
			"do { \\\n"
			"	st->state = _state; \\\n"
			"	goto PUSH_CHAR_DONE; \\\n"
			"} while(0);\n"
			"\n"
			"#define charset_has(cs, c) (c <= cs##_len && !!cs[c])\n"
		);
		if(print_guards) printf("#endif // %s_MACROS\n\n\n", guard_prefix);
	}
	
	
	if(print_enum_names) {
		if(print_guards) printf("#ifdef %s_ENUM_NAMES\n", guard_prefix);
		
		printf("// terminals\n"); 
		VEC_EACH(&tctx->terminals, i, t) {
			printf("[%s] = \"%s\",\n", t->name, t->name);
		}
		
		printf("\n// internals\n"); 
		VEC_EACH(&tctx->internals, i, t) {
			printf("[%s] = \"%s\",\n", t->name, t->name);
		}
		
		if(print_guards) printf("#endif // %s_ENUM_NAMES\n\n\n", guard_prefix);
	}
	
	if(print_enums) {
		if(print_guards) printf("#ifdef %s_ENUMS\n", guard_prefix);
		
		printf("// terminals\n"); 
		VEC_EACH(&tctx->terminals, i, t) {
			printf("%s,\n", t->name);
		}
		
		printf("\n// internals\n"); 
		VEC_EACH(&tctx->internals, i, t) {
			printf("%s,\n", t->name);
		}
		
		if(print_guards) printf("#endif // %s_ENUMS\n\n\n", guard_prefix);
	}
	
	if(print_data_defs) {
		if(print_guards) printf("#ifdef %s_TERMINAL_DATA_DEFS\n", guard_prefix);
		
		VEC_EACH(&tctx->terminals, i, t) {
			if(VEC_LEN(&t->terminal_data)) {
				printf("char* %s%s[] = {", data_def_prefix, t->name);
				
				VEC_EACH(&t->terminal_data, i2, dp) {
					char* k = dp.key ? dp.key : "";
					char* v = dp.val ? dp.val : "";
					printf("\"%s\", \"%s\", ", k, v); // TODO: string escaping
				}
				
				printf("NULL};\n");
			}
		}
		
		if(print_guards) printf("#endif // %s_TERMINAL_DATA_DEFS\n\n\n", guard_prefix);
	}
	
	if(print_data) {
		if(print_guards) printf("#ifdef %s_TERMINAL_DATA\n", guard_prefix);
		
		VEC_EACH(&tctx->terminals, i, t) {
			if(VEC_LEN(&t->terminal_data)) {
				printf("[%s] = %s%s,\n", t->name, data_def_prefix, t->name);
			}
		}
		
		if(print_guards) printf("#endif // %s_TERMINAL_DATA\n\n\n", guard_prefix);
	}
	
	
	if(print_unique_data) {
		if(print_guards) printf("#ifdef %s_UNIQUE_TERMINAL_PAIRS\n", guard_prefix);
		
		struct strtrip {
			char* sname, *key, *val;
		};
		
		HT(struct strpair*) table;
		HT_init(&table, VEC_LEN(&tctx->terminals) * 1.5);
		
		VEC_EACH(&tctx->terminals, i, t) {
			VEC_EACHP(&t->terminal_data, i2, tp) {
				
				char* k = malloc(strlen(tp->key) + 2 + strlen(tp->val));
				strcpy(k, tp->key);
				strcat(k, "=");
				strcat(k, tp->val);
				
				HT_set(&table, k, tp);
				free(k);
//				printf("[%s] = %s%s,\n", t->name, data_def_prefix, t->name);
			}
		}
		
		HT_EACH(&table, k, strpair*, tp) {
			printf("TDX(%s, %s)\n", tp->key, tp->val);
		}
		
		HT_destroy(&table);
		
		if(print_guards) printf("#endif // %s_UNIQUE_TERMINAL_PAIRS\n\n\n", guard_prefix);
	}
	
	
	
	if(print_triplet_data) {
		if(print_guards) printf("#ifdef %s_TERMINAL_TRIPLETS\n", guard_prefix);
		
		VEC_EACH(&tctx->terminals, i, t) {
			VEC_EACHP(&t->terminal_data, i2, tp) {
				
				printf("TDX(%s, %s, %s)\n", t->name, tp->key, tp->val);
			}
		}
		
		if(print_guards) printf("#endif // %s_TERMINAL_TRIPLETS\n\n\n", guard_prefix);
	}
	
	
	
	if(print_csets) {
		if(print_guards) printf("#ifdef %s_CSETS\n", guard_prefix);
		
		HT_EACH(&tctx->csets, key, charset*, cs) {
			printf("char cset_%s[] = {", cs->name);
			for(int i = 0; i < cs->minval; i++) printf("0,"); 
			for(int i = cs->minval; i <= cs->maxval; i++) printf("%d,", !!cs->table[i - cs->minval]); 
			printf("0};\n");
			printf("int cset_%s_len = %d;\n", cs->name, cs->maxval);
		}
		
		if(print_guards) printf("#endif // %s_CSETS\n\n\n", guard_prefix);
	}
	
	
	if(print_switch) {
		
		if(print_guards) printf("#ifdef %s_SWITCH\n", guard_prefix);
		
		printf("\n// terminals\n");
		VEC_EACH(&tctx->terminals, i, si) {
			print_state_switch(si);
		}
		
		printf("\n// internals\n");
		VEC_EACH(&tctx->internals, i, si) {
			print_state_switch(si);
		}
// 		HT_EACH(&tctx->states, key, state_info*, si) {
// 			print_state_switch(si);
// 			
// 		}
		if(print_guards) printf("#endif // %s_SWITCH\n\n\n", guard_prefix);
		
	}
	
	
	
	
	
	return 0;
}

