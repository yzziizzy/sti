#ifndef __sti__matrix_h__
#define __sti__matrix_h__


/*

Arbitrary sized 2-dimensional float matrix operations.

For a more extensive 4x4 and 3x3 matrix library (for 3d games),
see my other repo c3dlas.

Unlike most of sti, the matrix library is explicitly prefixed 
with "sti_" because "matrix" is such a popular data type name
in other libraries. I assume that there are people who will 
want to cherry pick the sti matrix files, or who want to use 
sti with their own matrix functions but don't want to have to 
think about removing the sti versions.  

*/



#include <stdio.h> // fprintf


typedef struct sti_matrix {
	int c, r;
	float data[0];
} sti_matrix;




sti_matrix* sti_matrix_new(int c, int r);
sti_matrix* sti_matrix_same_size(sti_matrix* m);
sti_matrix* sti_matrix_size_for_mul(sti_matrix* a, sti_matrix* b);

sti_matrix* sti_matrix_copy(sti_matrix* m);

// careful here...
void sti_matrix_print(sti_matrix* m, FILE* f);

void sti_matrix_clear(sti_matrix* m);
void sti_matrix_set(sti_matrix* m, float v);
void sti_matrix_load(sti_matrix* m, float* v);
void sti_matrix_ident(sti_matrix* m);

void sti_matrix_rand(sti_matrix* m, float min, float max);

void sti_matrix_transpose(sti_matrix* a, sti_matrix* out);

int sti_matrix_eq(sti_matrix* a, sti_matrix* b);

// returns a newly allocated matrix of the proper size
sti_matrix* sti_matrix_mul(sti_matrix* a, sti_matrix* b);
// no checks for size match.
void sti_matrix_mulp(sti_matrix* a, sti_matrix* b, sti_matrix* out);

// multiplies a with the transpose of b
sti_matrix* sti_matrix_mul_transb(sti_matrix* a, sti_matrix* b);
// no checks for size match.
void sti_matrix_mulp_transb(sti_matrix* a, sti_matrix* b, sti_matrix* out);



void sti_matrix_add(sti_matrix* a, sti_matrix* b, sti_matrix* out);

void sti_matrix_sub(sti_matrix* a, sti_matrix* b, sti_matrix* out);

void sti_matrix_scalar_mul(sti_matrix* a, sti_matrix* b, sti_matrix* out);
void sti_matrix_scale(sti_matrix* a, float s, sti_matrix* out);

// apply e^a[n]
void sti_matrix_exp(sti_matrix* a, sti_matrix* out);

// simple flat sum of all values in the matrix
float sti_matrix_sum(sti_matrix* a);


void sti_matrix_softmax(sti_matrix* a, sti_matrix* out);



void sti_matrix_min(sti_matrix* a, float minval, sti_matrix* out);
void sti_matrix_max(sti_matrix* a, float maxval, sti_matrix* out);
void sti_matrix_clamp(sti_matrix* a, float minval, float maxval, sti_matrix* out);

void sti_matrix_relu_0(sti_matrix* a, sti_matrix* out);
void sti_matrix_relu_half(sti_matrix* a, sti_matrix* out);
void sti_matrix_relu_n(sti_matrix* a, float n, sti_matrix* out);


// mean squared error: SUM( (a - b)^2 )
float sti_matrix_mse(sti_matrix* a, sti_matrix* b);


#endif // __sti__matrix_h__
