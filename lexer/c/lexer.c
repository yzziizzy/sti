

#include <stdlib.h>

#include <stdio.h> // only for debugging



#include "lexer.h"





#define A(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define D(c) (('0' <= (c) && (c) <= '9'))
#define AN(c) (A(c) || D(c))
#define WS(c) ((c) == ' ' || (c) == '\t' || (c) == '\v')
#define NL(c) ((c) == '\r' || (c) == '\n')


#define ESCAPED_LB(found, invalid) \
	if(s[0] == '\\') { \
		if(s[1] == '\r') { \
			if(s[2] == '\n') s++; \
			s += 2; \
			line++; \
			col = 0; \
			found; \
		} \
		else if(s[1] == '\n') { \
			s += 2; \
			line++; \
			col = 0; \
			found; \
		} \
		\
		invalid; \
	}


#define CHECK_BUF(t, n) \
do { \
	if(n >= (t)->alloc) { \
		(t)->alloc *= 2; \
		(t)->text = realloc((t)->text, (t)->alloc *  sizeof(*(t)->text)); \
	} \
} while(0);


int is_token(lexer_source_t* src, lexer_token_t* t) {
	char* s = src->head;
	char* os = s;
	
	char* buf = t->text;
	
	int line = t->start_line;
	int col = t->start_col;
	
	int was_e = 0;
	
RESTART:
	int n = 1;
	int c = s[0];
	
	buf[0] = c;
	switch(c) {
			case '\\':
				if(s[1] == '\r') {
					if(s[2] == '\n') s++;
					s += 2;
					line++;
					col = 0;
					goto RESTART;
				}
				else if(s[1] == '\n') {
					s += 2;
					line++;
					col = 0;
					goto RESTART;
				}
				// unicode escape sequences *in the code itself* are not supported. GTFO with that IOCCC shit.
				goto INVALID;
			
			
			case '\r':
			case '\n':
			case ' ':
			case '\t':
			case '\v':
				n = 0;
				while(1) {
					if(*s == ' ' || *s == '\t' || *s == '\v') {
						CHECK_BUF(t, n);
						buf[n] = *s;
						n++; s++; col++;
						continue;
					}
					
					if(*s == '\r') {
						CHECK_BUF(t, n);
						buf[n++] = *s;
						s++;
						
						if(*s == '\n') {
							CHECK_BUF(t, n);
							buf[n++] = *s; 
							s++;
						}
						
						line++;
						col = 0;
						t->has_newline = 1;
						continue;
					}
					
					if(*s == '\n') {
						CHECK_BUF(t, n);
						buf[n++] = *s;
						s++;
						
						line++;
						col = 0;
						t->has_newline = 1;
						continue;
					}
					
					ESCAPED_LB(continue, );
					
					break;
				}
				
				t->type = LEXER_TOK_SPACE;
				goto RETURN;
			
			
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': 
			case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': 
			case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': 
			case 'v': case 'w': case 'x': case 'y': case 'z': 
			case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': 
			case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': 
			case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': 
			case 'V': case 'W': case 'X': case 'Y': case 'Z':
			case '_': 
				goto IDENT;
			
			
			case '\'':
				goto CHARLIT;
				
			case '"':
				goto STRING;
			
			case '[': case ']':
			case '{': case '}':
			case '(': case ')':
			case '?': case ';':
			case '~': case ',':
				goto PUNCT1;
			
			case '*':
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '#':
				if(s[1] == '#') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;		
			
			case '!':
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;		
			
			case '^':
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;		
						
			case ':':
				if(s[1] == '>') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
			
			case '=':
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
			
			case '+':
				if(s[1] == '=' || s[1] == '+') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '|':
				if(s[1] == '=' || s[1] == '|') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;		
			
			case '&':
				if(s[1] == '=' || s[1] == '&') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;		
			
			case '%':
				if(s[1] == ':') {
					if(s[2] == '%' && s[3] == ':') goto PUNCT4;
					if(s[2] == '%' && s[3] == '\\') goto SLOW;
					if(s[2] == '\\') goto SLOW;
					goto PUNCT2;
				}				
				if(s[1] == '=' || s[1] == '>') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;

			case '-':
				if(s[1] == '=' || s[1] == '>' || s[1] == '-') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '/':
				if(s[1] == '/') goto SL_COMMENT;
				if(s[1] == '*') goto ML_COMMENT;
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '>':
				if(s[1] == '=') goto PUNCT2;
				if(s[1] == '>') {
					if(s[2] == '=') goto PUNCT3;
					if(s[2] == '\\') goto SLOW;
					goto PUNCT2;
				}
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '<':
				if(s[1] == '=' || s[1] == ':' || s[1] == '%') goto PUNCT2;
				if(s[1] == '<') {
					if(s[2] == '=') goto PUNCT3;
					if(s[2] == '\\') goto SLOW;
					goto PUNCT2;
				}
				if(s[1] == '\\') goto SLOW;
				goto PUNCT1;
				
			case '.':
				if(s[1] == '.') {
					if(s[2] == '.') goto PUNCT3;
					if(s[2] == '\\') goto SLOW;
					goto PUNCT1; // only return the first '.'
				}
				if(s[1] == '\\') goto SLOW;
				goto NUMBER;
			
			case '0': case '1': case '2': case '3': case '4': 
			case '5': case '6': case '7': case '8': case '9':
				goto NUMBER;	
				
			default:
				goto INVALID;
	}
	
IDENT:
	s++;
	while(1) {
		if(AN(*s) || *s == '_') {
			CHECK_BUF(t, n);
			buf[n] = *s;
			n++; s++; col++;
			continue;
		}
		
		ESCAPED_LB(continue, );
		
		break;
	}
	
	t->type = LEXER_TOK_IDENT;
	goto RETURN;


NUMBER:
	s++;
	while(1) {
		ESCAPED_LB(continue, );
	
		if(was_e) {
			if(*s == '+' || *s == '-') {
				CHECK_BUF(t, n);
				buf[n] = *s;
				n++; s++; col++;
				continue;
			}
			
			was_e = 0;
		}
		
		if((n == 1 && D(*s)) || (n > 1 && AN(*s))) {
			CHECK_BUF(t, n);
			buf[n] = *s;
			n++; s++; col++;
			continue;
		}
		
		if(*s == 'e' || *s == 'E' || *s == 'p' || *s == 'P') {
			CHECK_BUF(t, n);
			buf[n++] = *s;
			s++; col++;
			
			was_e = 1;
		}
		
		
		break;
	}
	
	if(n == 1 && buf[0] == '.') goto PUNCT1;
	
	t->type = LEXER_TOK_NUMBER;
	goto RETURN;
	
	
SL_COMMENT:
	s++;
	while(1) {
		if(*s == '\r' || *s == '\n') {
			break;
		}
		
		ESCAPED_LB(continue, );
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	t->type = LEXER_TOK_COMMENT;
	goto RETURN;


ML_COMMENT: {
	s++;
	int state = 0; 
	while(1) {
		ESCAPED_LB(continue, );
		if(state == 1) {
			if(*s == '/') {
				CHECK_BUF(t, n);
				buf[n++] = *s;
				s++; col++;
				break;
			}
			
			state = 0;
		}
		
		if(*s == '*') state = 1;
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	t->type = LEXER_TOK_COMMENT;
	goto RETURN;
}


STRING: {
	int state = 0;
	s++;
	while(1) {
		if(state == 1) {
			state = 0;
			
			if(*s == '"') {
				CHECK_BUF(t, n);
				buf[n++] = *s;
				s++; col++;
				continue;
			}
		}
		
		if(*s == '"') break;
		
		ESCAPED_LB(continue, state = 1);
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	CHECK_BUF(t, n);
	buf[n++] = *s;
	s++; col++;
	
	t->type = LEXER_TOK_STRING;
	goto RETURN;
}

CHARLIT: {
	s++;
	int state = 0; 
	while(1) {
		if(state == 1) {
			state = 0;
			
			if(*s == '\'') {
				CHECK_BUF(t, n);
				buf[n++] = *s;
				s++; col++;
				continue;
			}
		}
		
		if(*s == '\'') break;
		
		ESCAPED_LB(continue, state = 1);
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	CHECK_BUF(t, n);
	buf[n++] = *s;
	s++; col++;
	
	t->type = LEXER_TOK_CONST;
	goto RETURN;
}



PUNCT4:
	
	buf[3] = s[3];
	buf[2] = s[2];
	buf[1] = s[1];
	n = 4;
	goto PUNCT1;
	
PUNCT3:
	buf[2] = s[2];
	buf[1] = s[1];
	n = 3;
	goto PUNCT1;

PUNCT2:
	buf[1] = s[1];
	n = 2;
	goto PUNCT1;
	
PUNCT1:
	t->type = LEXER_TOK_PUNCT;
	goto RETURN;
		
INVALID:
	t->type = LEXER_TOK_INVALID;
	goto RETURN;
	
SLOW:
	// Escaped linebreaks are rare but also a pain in the ass.
	// There's a dedicated "correct" function for processing them.
	// It only needs to handle the multicharacter punctuation.
	return is_token_slow(src, t);
	
RETURN:
	t->end_line = line;
	t->end_col = col;
	t->len = n;
	src->head = s;
	
	return n;
}













// states
enum {
	_NONE = 0,
	_STAR,
	_HASH,
	_BANG,
	_CARET,
	_COLON,
	_EQUAL,
	_PLUS,
	_PIPE,
	_AMP,
	_PCT,
	_PCT_COLON,
	_PCT_COLON_PCT,
	_MINUS,
	_SLASH,
	_GT,
	_GT_GT,
	_LT,
	_LT_LT,
	_DOT,
	_DOT_DOT,
};



	
int is_token_slow(lexer_source_t* src, lexer_token_t* t) {
	char* s = src->head;
	char* os = s;
	
	char* buf = t->text;
	
	int line = t->start_line;
	int col = t->start_col;
	
	int was_e = 0;
	int n = 0;
	
	int state = _NONE;
	
	printf("slow method \n");
START:

	int c = s[0];
	
	if(*s == '\\') {
		printf("escape\n");
		if(s[1] == '\r') {
			if(s[2] == '\n') s++;
			s += 2;
			line++;
			col = 0;
			goto START;
		}
		else if(s[1] == '\n') {
			printf("enl\n");
			s += 2;
			line++;
			col = 0;
			goto START;
		}
		// unicode escape sequences *in the code itself* are not supported. GTFO with that IOCCC shit.
		goto INVALID;
	}
	
//	buf[n++] = *s;
	printf("c='%c', n=%d\n", *s, n);
	switch(state) {
		case _NONE:
			switch(*s) {
				case '*': state = _STAR; goto RESTART;
				case '#': state = _HASH; goto RESTART;
				case '!': state = _BANG; goto RESTART;
				case '^': state = _CARET; goto RESTART;
				case ':': state = _COLON; goto RESTART;
				case '=': state = _EQUAL; goto RESTART;
				case '+': state = _PLUS; goto RESTART;
				case '|': state = _PIPE; goto RESTART;
				case '&': state = _AMP; goto RESTART;
				case '%': state = _PCT; goto RESTART;
				case '-': state = _MINUS; goto RESTART;
				case '/': state = _SLASH; goto RESTART;
				case '>': state = _GT; goto RESTART;
				case '<': state = _LT; goto RESTART;
				case '.': state = _DOT; goto RESTART;
			}	
			
		case _STAR:
			if(s[0] == '=') goto PUNCT2;
			goto PUNCT1;
			
		case _HASH:
			if(s[0] == '#') goto PUNCT2;
			goto PUNCT1;		
		
		case _BANG:
			if(s[0] == '=') goto PUNCT2;
			goto PUNCT1;		
		
		case _CARET:
			if(s[0] == '=') goto PUNCT2;
			goto PUNCT1;		
					
		case _COLON:
			if(s[0] == '>') goto PUNCT2;
			goto PUNCT1;
		
		case _EQUAL:
			if(s[0] == '=') goto PUNCT2;
			goto PUNCT1;
		
		case _PLUS:
			if(s[0] == '=' || s[0] == '+') goto PUNCT2;
			goto PUNCT1;
			
		case _PIPE:
			if(s[0] == '=' || s[0] == '|') goto PUNCT2;
			goto PUNCT1;		
		
		case _AMP:
			if(s[0] == '=' || s[0] == '&') goto PUNCT2;
			goto PUNCT1;		
		
		case _PCT:
			if(s[0] == '=' || s[0] == '>') goto PUNCT2;
			if(s[0] == ':') { state = _PCT_COLON; goto RESTART; }
			goto PUNCT1;
		
		case _PCT_COLON:
			if(s[0] == '%') { state = _PCT_COLON_PCT; goto RESTART; }
			goto PUNCT2;
			
		case _PCT_COLON_PCT:
			if(s[0] == ':') goto PUNCT4;
			goto PUNCT2;

		case _MINUS:
			if(s[0] == '=' || s[0] == '>' || s[0] == '-') goto PUNCT2;
			goto PUNCT1;
			
		case _SLASH:
			if(s[0] == '/') goto SL_COMMENT;
			if(s[0] == '*') goto ML_COMMENT;
			if(s[0] == '=') goto PUNCT2;
			goto PUNCT1;
			
		case _GT:
			if(s[0] == '=') goto PUNCT2;
			if(s[0] == '>') { state = _GT_GT; goto RESTART; }
			goto PUNCT1;
			
		case _GT_GT:
			if(s[0] == '=') goto PUNCT3;
			goto PUNCT2;
		
		case _LT:
			if(s[0] == '=') goto PUNCT2;
			if(s[0] == '<') { state = _LT_LT; goto RESTART; }
			goto PUNCT1;
			
		case _LT_LT:
			if(s[0] == '=') goto PUNCT3;
			goto PUNCT2;
			
		case _DOT:
			printf("dot\n");
			if(s[0] == '.') { state = _DOT_DOT; goto RESTART; }
			goto NUMBER;
		
		case _DOT_DOT:
			printf("dot_dot\n");
			if(s[0] == '.') goto PUNCT3;
			goto PUNCT1;	
	}
	
	
RESTART:
	CHECK_BUF(t, n);
	buf[n++] = *s;
	s++; col++;
	goto START;


PUNCT4:
//	buf[3] = s[3];
//	buf[2] = s[2];
//	buf[1] = s[1];
//	n = 4;
	goto PUNCT1;
	
PUNCT3:
//	buf[2] = s[2];
//	buf[1] = s[1];
//	n = 3;
	goto PUNCT1;

PUNCT2:
//	buf[1] = s[1];
//	n = 2;
	goto PUNCT1;
	
PUNCT1:
	buf[n++] = *s;
	t->type = LEXER_TOK_PUNCT;
	goto RETURN;
	

NUMBER:
	printf("num--------\n");
	while(1) {
		printf("%c n=%d\n", *s, n);
		ESCAPED_LB(continue, );
	
		if(was_e) {
			if(*s == '+' || *s == '-') {
				CHECK_BUF(t, n);
				buf[n] = *s;
				n++; s++; col++;
				continue;
			}
			
			was_e = 0;
		}
		
		if(n == 1 && !D(*s)) {
			t->type = LEXER_TOK_PUNCT;
			goto RETURN;
		}
		
		if(n > 1 && AN(*s)) {
			CHECK_BUF(t, n);
			buf[n] = *s;
			n++; s++; col++;
			continue;
		}
		
		if(*s == 'e' || *s == 'E' || *s == 'p' || *s == 'P') {
			CHECK_BUF(t, n);
			buf[n++] = *s;
			s++; col++;
			
			was_e = 1;
		}
		
		
		break;
	}
	
	if(n == 1 && buf[0] == '.') goto PUNCT1;
	
	t->type = LEXER_TOK_NUMBER;
	goto RETURN;
	
SL_COMMENT:
	while(1) {
		if(*s == '\r' || *s == '\n') {
			break;
		}
		
		ESCAPED_LB(continue, );
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	t->type = LEXER_TOK_COMMENT;
	goto RETURN;


ML_COMMENT: {
	s++;
	int state = 0; 
	while(1) {
		ESCAPED_LB(continue, );
		if(state == 1) {
			if(*s == '/') {
				CHECK_BUF(t, n);
				buf[n++] = *s;
				s++; col++;
				break;
			}
			
			state = 0;
		}
		
		if(*s == '*') state = 1;
		
		CHECK_BUF(t, n);
		buf[n++] = *s;
		s++; col++;
	}
	
	t->type = LEXER_TOK_COMMENT;
	goto RETURN;
}
INVALID:
	t->type = LEXER_TOK_INVALID;
	goto RETURN;

RETURN:
	t->end_line = line;
	t->end_col = col;
	t->len = n;
	src->head = s;
	
	return n;
}



char* lexer_token_type_names[] = {
#define X(a) [LEXER_TOK_##a] = #a,
	LEXER_TOKEN_TYPE_LIST
#undef X
};




