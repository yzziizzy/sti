


#include "matrix.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h> // malloc
#include <string.h> // memset



#define M(m, _c, _r) m->data[(_c) + (_r) * m->c]



sti_matrix* sti_matrix_new(int c, int r) {
	sti_matrix* mat;
	
	mat = malloc(sizeof(*mat) + sizeof(mat->data[0]) * r * c);
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



// careful here...
void sti_matrix_print(sti_matrix* m, FILE* f) {
	for(long r = 0; r < m->r; r++) {
		for(long c = 0; c < m->c; c++) {
			fprintf(f, "%.2f ", m->data[c + m->c * r]);
		}
		
		fprintf(f, "\n");
	}
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


void sti_matrix_load(sti_matrix* m, float* v) {
	memcpy(m->data, v, sizeof(m->data[0]) * m->c * m->r);
}


void sti_matrix_ident(sti_matrix* m) {
	for(int i = 0; i < m->c; i++)
	for(int j = 0; j < m->r; j++) {
		m->data[i + j * m->c] = i == j;
	}
}



void sti_matrix_rand(sti_matrix* m, float min, float max) {
	long len = m->c * m->r;
	float sz = max - min;
	for(long n = 0; n < len; n++) {
		float x = ((float)rand() * sz) / (float)RAND_MAX;
		m->data[n] = min + x; 
	}
}


void sti_matrix_transpose(sti_matrix* a, sti_matrix* out) {
	
	assert(a->c * a->r <= out->c * out->r);
	
	out->r = a->c;
	out->c = a->r;
	
	for(int r = 0; r < a->r; r++)
	for(int c = r; c < a->c; c++) {
		float tmp;
		if(c < a->c) tmp = M(a, c, r);
		if(c < out->c) M(out, c, r) = M(a, r, c);
		if(c < a->c) M(out, r, c) = tmp;
	}
}


int sti_matrix_eq(sti_matrix* a, sti_matrix* b) {
	if(a->r != b->r || a->c != b->c) return 0;
	
	long len = a->c * a->r;
	for(long n = 0; n < len; n++) {
		if(a->data[n] != b->data[n]) return 0; 
	}
	
	return 1;
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
	
	long klim = a->c;
	
	for(int c = 0; c < b->c; c++)
	for(int r = 0; r < a->r; r++) {
		M(out, c, r) = 0;
		for(int k = 0; k < klim; k++) {
			M(out, c, r) += M(a, k, r) * M(b, c, k);
		}
	}
}


// multiplies a with the transpose of b
sti_matrix* sti_matrix_mul_transb(sti_matrix* a, sti_matrix* b) {
	sti_matrix* o;
	
	if(a->c != b->c) return NULL;
	
	o = sti_matrix_new(b->r, a->r);
	sti_matrix_mulp_transb(a, b, o);
	
	return o;
}


void sti_matrix_mulp_transb(sti_matrix* a, sti_matrix* b, sti_matrix* out) {
	
	long klim = a->c;
	
	for(int c = 0; c < b->r; c++)
	for(int r = 0; r < a->r; r++) {
		M(out, c, r) = 0;
		for(int k = 0; k < klim; k++) {
			M(out, c, r) += M(a, k, r) * M(b, k, c);
		}
	}
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


void sti_matrix_scalar_mul(sti_matrix* a, sti_matrix* b, sti_matrix* out) {

	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = a->data[i] * b->data[i];
	}
}

void sti_matrix_scale(sti_matrix* a, float s, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = a->data[i] * s;
	}
}



// apply e^a[n]
void sti_matrix_exp(sti_matrix* a, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = expf(a->data[i]);
	}
}

// simple flat sum of all values in the matrix
float sti_matrix_sum(sti_matrix* a) {
	
	long sz = a->c * a->r;
	float sum = 0;
	
	for(int i = 0; i < sz; i++) {
		sum += a->data[i];
	}
	
	return sum;
}


void sti_matrix_softmax(sti_matrix* a, sti_matrix* out) {
	
	long sz = a->c * a->r;
	float sum = 0;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = expf(a->data[i]);
		sum += out->data[i];
	}
	
	float invsum = 1.0 / sum;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] *= invsum;
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


void sti_matrix_relu_0(sti_matrix* a, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fmax(0, a->data[i]);
	}
}

void sti_matrix_relu_half(sti_matrix* a, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fmax(0, a->data[i] - .5f) + .5f;
	}
}

void sti_matrix_relu_n(sti_matrix* a, float n, sti_matrix* out) {
	
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = fmax(0, a->data[i] - n) + n;
	}
}

void sti_matrix_silu(sti_matrix* a, sti_matrix* out) {
	long sz = a->c * a->r;
	
	for(int i = 0; i < sz; i++) {
		out->data[i] = a->data[i] / (1.f + expf(-a->data[i]));
	}
}


// mean squared error: SUM( (a - b)^2 )
float sti_matrix_mse(sti_matrix* a, sti_matrix* b) {

	long sz = a->c * a->r;
	
	float sum = 0;
	for(int i = 0; i < sz; i++) {
		float x = a->data[i] - b->data[i];
		sum += x * x;
	}
	
	return sum / sz;
}


