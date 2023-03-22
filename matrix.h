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
sti their own matrix functions but don't want to have to think
about removing the sti versions.  

*/


// column major: [c1r1, c1r2, c1r3, ..., c2r1, c2r2, c2r3, ..., c3r1, c3r2, c3r3, ..., ...]



typedef struct sti_matrix {
	int c, r;
	float data[0];
} sti_matrix;




sti_matrix* sti_matrix_new(int c, int r);
sti_matrix* sti_matrix_same_size(sti_matrix* m);
sti_matrix* sti_matrix_size_for_mul(sti_matrix* a, sti_matrix* b);

sti_matrix* sti_matrix_copy(sti_matrix* m);

void sti_matrix_clear(sti_matrix* m);
void sti_matrix_set(sti_matrix* m, float v);
void sti_matrix_ident(sti_matrix* m);

void sti_matrix_transpose(sti_matrix* a, sti_matrix* out);

// returns a newly allocated matrix of the proper size
sti_matrix* sti_matrix_mul(sti_matrix* a, sti_matrix* b);

// multiplies a with the transpose of b
sti_matrix* sti_matrix_mul_transb(sti_matrix* a, sti_matrix* b);

// no checks for size match.
void sti_matrix_mulp(sti_matrix* a, sti_matrix* b, sti_matrix* out);


void sti_matrix_add(sti_matrix* a, sti_matrix* b, sti_matrix* out);

void sti_matrix_sub(sti_matrix* a, sti_matrix* b, sti_matrix* out);
void sti_matrix_scalar_mul(sti_matrix* a, float s, sti_matrix* out);


void sti_matrix_min(sti_matrix* a, float minval, sti_matrix* out);
void sti_matrix_max(sti_matrix* a, float maxval, sti_matrix* out);
void sti_matrix_clamp(sti_matrix* a, float minval, float maxval, sti_matrix* out);


#endif // __sti__matrix_h__
