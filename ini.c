
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fs.h"
#include "ini.h"





void ini_read(char* path, ini_read_callback_fn fn, void* user_data) {
	
	size_t src_sz;
	char* src, *s, *src_end, *e, *h;
	char* section_name = "";
	char* key, *value;
	char quote_c;
	
	src = readWholeFileExtra(path, 1, &src_sz);
	src[src_sz] = '\n';
	
	// technically it should handle embedded nulls
	src_end = src + src_sz;
	s = src;
	
	
	while(s < src_end) {
		char c = *s;
		
		if(isspace(c) || c == 0) {
			s++; continue;
		}
		
		// section headers
		if(c == '[') {
			e = strchr(s, ']');
			section_name = s + 1;
			*e = 0;
			s = e + 1;
			continue;
		}
	
		// find the end of the key
		h = s;
		e = s;
		key = s;
		value = NULL;
		while(1) {
			if(h >= src_end) {
				// valueless last key in the file
				e[1] = 0;
				s = h + 1;
				goto VALUELESS;
			}
			
			// key with no value
			if(*h == '\n') {
				e[1] = 0;
				s = h + 1;
				goto VALUELESS;
			}
			
			if(*h == '=') {
				// end of key
				break;
			}
						
			if(!isspace(*h)) e = h;
			
			h++;
		}
		
		e[1] = 0;
		s = h + 1;
		
		// skip to te first bit of content
		s += strspn(s, " \t\r\n");
		
		// find the end of the value
		if(*s == '"' || *s == '\'') {
			quote_c = *s;
			s++;
		}
		else quote_c = '\n';
		
		h = s;
		e = s;
		while(1) {
			if(h >= src_end) {
				// found the end
				break;
			}
			
			if(*h == quote_c) break;
				
			if(!isspace(*h)) e = h;
			
			h++;
		}
		
		e[1] = 0;
		value = s == e ? NULL : s;
		
	VALUELESS:
		
		if(fn(section_name, key, value, user_data)) break;
	
		s = e + 1;
	}
	
	free(src);
}




