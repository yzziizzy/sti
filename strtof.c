

float strtof_1(char* s, char** end) {
	
	// TODO: cut the calculations past the precision of floats (save MS power for reference)
	// TODO: try caching the divisors
	
	char* endp;
	
	// figure out the exponent first
	int e_power = 0;
	int e_neg = 1;
	for(int i = 0; ; i++) {
		
		if(s[i] == 'e' || s[i] == 'E') {;
			i++;
			
			// handle signs
			if(s[i] == '-') {
				e_neg = -1;
				i++;
			}
			else if(s[i] == '+') {
				i++;
			}
			
			int j;
			for(j = i; ; j++) {
				int c = s[j] - '0';
				if(c < 0 || c > 9) break;
				e_power *= 10;
				e_power += c;
			}
			
			endp = s + j;
			goto FOUND_E;
		}
		
		if(s[i] == 0) { // end of string
			endp = s + i;
			break;
		}
		
		// only scan past number characters
		int c = s[i] - '0';
		if(!(c >= 0 || c <= 9) && s[i] == '.' && s[i] != '-' && s[i] != '+') {
			endp = s + i;
			break;
		}
	}
	
FOUND_E:
	
	e_power *= e_neg;
	
//	printf("eneg> %d\n", e_neg);
//	printf("e> %d\n", e_power);
	
	double power;
	
	// integer part
	double neg = 1;
	double d = 0;
	if(*s == '-') neg = -1;
	if(*s == '+') s++;
	
	while(1) {
		if(!*s) goto DONE;
		if(*s == '.') break;
		if(*s == 'e') break;
		if(*s == 'E') break;
		
		int c = *s - '0';
		if(c > 9 || c < 0) goto DONE;
		d *= 10;
		d += c;
		
//		printf("d> %.15f\n",d);
		
		s++;
	}
	
//	printf("period\n");
	s++;
	
	// decimals
	power = 10;
	
	
	while(*s) {
		int c = *s - '0';
		if(c > 9 || c < 0) break;
		
		double divisor = 1.0 / power;
		double n = c * divisor;
		d += n;
	
//		printf("d> %.15f\n",d);
	
		power *= 10;
		s++;
	}
//	printf("d> %.15f\n",d);
	
	if(e_power > 0) {
//		printf("e> %d\n", e_power);
		for(int i = 0; i < e_power; i++) {
			d *= 10;
		}
	}
	else if(e_power < 0) {
//		printf("e?> %d\n", e_power);
		for(int i = 0; i < -e_power; i++) {
			d /= 10;
		}
	}
	
//	printf("d> %.15f\n",d);

DONE:
	if(end) *end = endp;
	
	return d * neg;
}



