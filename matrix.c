


#include "matrix.h"




#define M(m, _c, _r) m->data[(_c) + (_r) * m->c]



sti_matrix* sti_matrix_new(int c, int r) {
	sti_matrix* mat;
	
	mat = malloc(sizeof(*mat) + sizeof(mat->data) * r * c);
	mat->r = r;
	mat->c = c;
	
	return mat;
}


sti_matrix* sti_matrix_same_size(sti_matrix* m) {
	return sti_matrix_new(m->c, m->r);
}


sti_matrix* sti_matrix_size_for_mul(sti_matrix* a, sti_matrix* b) {
	return sti_matrix_new(b->c, a->r);
}


sti_matrix* sti_matrix_copy(sti_matrix* m) {
	sti_matrix* mat = sti_matrix_same_size(m);
	memcpy(mat->data, m->data, sizeof(*mat->data) * m->r * m->c);
	return mat;
}


void sti_matrix_clear(sti_matrix* m) {
	memset(m->data, 0, sizeof(m->data) * m->c * m->r);
}


void sti_matrix_set(sti_matrix* m, float v) {
	
	if(v == 0) {
		memset(m->data, 0, sizeof(m->data) * m->c * m->r);
		return;
	}
	
	long sz = m->c * m->r;
	
	for(int i = 0; i < sz; i++) {
		m->data[i] = v;
	}
}

void sti_matrix_ident(sti_matrix* m) {
	for(int i = 0; i < m->c; i++)
	for(int j = 0; j < m->r; j++) {
		m->data[i + j * m->c] = i == j;
	}
}


void sti_matrix_transpose(sti_matrix* a, sti_matrix* out) {
	float tmp;
	
	assert(a->c * a->r <= out->c * out->r);
	
	out->r = a->c;
	out->c = a->r;
	
	for(int i = 0; i < a->c; i++)
	for(int j = 0; j <= a->r; j++) {
		M(out, i, j) = M(a, j, i);
	}
}

sti_matrix* sti_matrix_mul(sti_matrix* a, sti_matrix* b) {
	sti_matrix* o;
	
	if(a->c != b->r) return NULL;
	
	o = sti_matrix_new(b->c, a->r);
	sti_matrix_mulp(a, b, o);
	
	return o;
}


// no checks for size match.
void sti_matrix_mulp(sti_matrix* a, sti_matrix* b, sti_matrix* out) {
	
	memset(out->data, 0, sizeof(out->data) * out->c * out->r);
	
	for(int j = 0; j < b->r; j++)
	for(int i = 0; i < a->c; i++) {
		for(int k = 0; k < a->r; k++) {
			// a moves right in columns
			// b moves down in rows
			M(out, i, j) += M(a, k, i) * M(b, j, k);
		}
	}
}


// multiplies a with the transpose of b
sti_matrix* sti_matrix_mul_transb(sti_matrix* a, sti_matrix* b) {
	sti_matrix* o;
	
	if(a->c != b->c) return NULL;
	
	o = sti_matrix_new(b->r, a->r);
	
	memset(o->data, 0, sizeof(o->data) * o->c * o->r);
	
	for(int j = 0; j < b->r; j++)
	for(int i = 0; i < a->c; i++) {
		for(int k = 0; k < a->r; k++) {
			M(o, i, j) += M(a, k, i) * M(b, j, k);
		}
	}
	
	return o;
}

#define MIN(a, b) (a < b ? a : b)

void sti_matrix_add(sti_matrix* a, sti_matrix* b, sti_matrix* out) {
	
	int c = MIN(out->c, MIN(a->c, b->c));
	int r = MIN(out->r, MIN(a->r, b->r));
	
	for(int j = 0; j < r; j++)
	for(int i = 0; i < c; i++) {
		M(out, i, j) = M(a, i, j) + M(a, i, j);
	}
}


void sti_matrix_sub(sti_matrix* a, sti_matrix* b, sti_matrix* out) {
	
	int c = MIN(out->c, MIN(a->c, b->c));
	int r = MIN(out->r, MIN(a->r, b->r));
	
	for(int j = 0; j < r; j++)
	for(int i = 0; i < c; i++) {
		M(out, i, j) = M(a, i, j) - M(a, i, j);
	}
}


void sti_matrix_scalar_mul(sti_matrix* a, float s, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = a->data[i] * s;
	}
}


void sti_matrix_min(sti_matrix* a, float minval, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fminf(a->data[i], minval);
	}
}


void sti_matrix_max(sti_matrix* a, float maxval, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fmaxf(a->data[i], maxval);
	}
}


void sti_matrix_clamp(sti_matrix* a, float minval, float maxval, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fminf(minval, fmaxf(a->data[i], maxval));
	}
}

void sti_matrix_tanh_clamp(sti_matrix* a, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = tanhf(a->data[i]);
	}
}








