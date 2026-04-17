
#include "qd.h"


typedef struct {
	double d[5];
} quad_double_temp_t;




void quick_two_sum(double a, double b, double* s, double* e) {
	*s = a + b;
	*e = b - (*s - a);
}

void two_sum(double a, double b, double* s, double* e) {
	*s = a + b;
	double v = *s - a;
	*e = (a - (*s - v)) + (b - v);
}

void three_sum_3(double x, double y, double z, double* r0, double* r1, double* r2) {
	double u, v, w;
	two_sum(x, y, &u, &v);
	two_sum(z, u, r0, &w);
	two_sum(v, w, r1, r2);
}

void three_sum_2(double x, double y, double z, double* r0, double* r1) {
	double u, v, w;
	two_sum(x, y, &u, &v);
	two_sum(z, u, r0, &w);
	*r1 = w + v;
}

void three_sum_1(double x, double y, double z, double* r0) {
	*r0 = (x + y) + z;
}

void six_three_sum(double a0, double a1, double a2, double a3, double a4, double a5, double* r0, double* r1, double* r2) {
	double j0, j1, j2, k0, k1, k2, e0, e1, e2, s1, s2;
	three_sum_3(a0, a1, a2, &j0, &j1, &j2);
	three_sum_3(a3, a4, a5, &k0, &k1, &k2);
	two_sum(j0, k0, r0, &e0);
	two_sum(j1, k1, &s1, &e1);
	s2 = j2 + k2;
	two_sum(e0, s1, r1, &e2);
	three_sum_1(s2, e1, e2, r2);
}


// This is double-double addition, but it's not clear if a final quick_two_sum() is needed here. The paper hand-waves it to a previous
//   paper which now has a dead link and has zero results in search engines or the (same) author's website. Sloppy paper-writing.
void four_two_sum(double a0, double a1, double a2, double a3, double* r0, double* r1) {
	double e1;
	two_sum(a0, a2, r0, &e1); 
	*r1 = e1 + a1 + a3;
}

void nine_two_sum(double a0, double a1, double a2, double a3, double a4, double a5, double a6, double a7, double a8, double* r0, double* r1) {
	double e0, e1, s0, s1, f0, f1, g0, g1, g2, g3;
	
	
	two_sum(a2, a3, &s0, &e0);
	two_sum(a1, a4, &s1, &e1);
	four_two_sum(s0, e0, s1, e1, &g0, &g1); 
	
	two_sum(a5, a7, &s0, &e0);
	two_sum(a6, a8, &s1, &e1);
	four_two_sum(s0, e0, s1, e1, &g2, &g3); 
	
	four_two_sum(g0, g1, g2, g3, &f0, &f1);
	
	three_sum_2(f0, f1, a0, r0, r1);
}

void split(double a, double* hi, double* lo) {
	double t = (2.e27 + 1.) * a;
	*hi = t - (t - a);
	*lo = a - *hi;
}


void two_prod(double a, double b, double* p, double *e) {
	*p = a * b;
	double ahi, alo, bhi, blo;
	split(a, &ahi, &alo);
	split(b, &bhi, &blo);
	*e = ((ahi * bhi - *p) + ahi * blo + alo * bhi) + alo * blo;
}

/*
void two_prod_fma(double a, double b, double* p, double *e) {
	*p = a * b;
	*e = fma(a * b - p);
}
*/



void renormalize(quad_double_temp_t* u, quad_double_t* n) {
	double s, e;
	quad_double_temp_t t;
	two_sum(u->d[3], u->d[4], &s,      &t.d[4]);
	two_sum(u->d[2], s,       &s,      &t.d[3]);
	two_sum(u->d[1], s,       &s,      &t.d[2]);
	two_sum(u->d[0], s,       &t.d[0], &t.d[1]);
	
	*n = (quad_double_t){};
	
	s = t.d[0];
	for(int i = 1, k = 0; i <= 4; i++) {
		quick_two_sum(s, t.d[i], &s, &e);
		if(e != 0) {
			n->d[k] = s;
			s = e;
			k++;
		}
	}
}


void qd_add(quad_double_t* a, quad_double_t* b, quad_double_t* sum) {
	quad_double_temp_t t;
	
	double e0, e1, e2, e3;
	double s1, s2, s3;
	double r0, f2, f3, g1;
	
	two_sum(a->d[0], b->d[0], &t.d[0], &e0);
	two_sum(a->d[1], b->d[1], &s1, &e1);
	two_sum(a->d[2], b->d[2], &s2, &e2);
	two_sum(a->d[3], b->d[3], &s3, &e3);
	
	
	two_sum(e0, s1, &t.d[1], &r0);
	
	three_sum_3(s2, e1, r0, &t.d[2], &f2, &f3);
	three_sum_2(s3, e2, f2, &t.d[3], &g1);
	three_sum_1(e3, g1, f3, &t.d[4]);
	
	renormalize(&t, sum);
}

void qd_neg(quad_double_t* a, quad_double_t* b) {
	for(int i = 0; i < 4; i++) b->d[i] = -a->d[i];
}

void qd_sub(quad_double_t* a, quad_double_t* b, quad_double_t* diff) {
	quad_double_t t;
	qd_neg(b, &t);
	qd_add(a, &t, diff);
}



void qd_mul_double(quad_double_t* a, double b, quad_double_t* prod) {
	quad_double_temp_t t;
	
	double e0, e1, e2;
	double s1, s2, s3;
	double r0, f2, f3, g1;
	
	two_prod(a->d[0], b, &t.d[0], &e0);
	two_prod(a->d[1], b, &s1, &e1);
	two_prod(a->d[2], b, &s2, &e2);
	s3 = a->d[3] * b;
	
	
	two_sum(e0, s1, &t.d[1], &r0);
	
	three_sum_3(s2, e1, r0, &t.d[2], &f2, &f3);
	three_sum_2(s3, e2, f2, &t.d[3], &g1);
	t.d[4] = g1 + f3;
	
	renormalize(&t, prod);
}





void qd_mul(quad_double_t* a, quad_double_t* b, quad_double_t* prod) {
	quad_double_temp_t t;
	double oe2, oe3_0, oe3_1, oe4_0, oe4_1;
	
	#define PQ(i, j) \
		double p##i##j, q##i##j; two_prod(a->d[i], b->d[j], &p##i##j, &q##i##j);
	PQ(0, 0)
	PQ(0, 1)
	PQ(0, 2)
	PQ(0, 3)
	PQ(1, 0)
	PQ(1, 1)
	PQ(1, 2)
	PQ(1, 3)
	PQ(2, 0)
	PQ(2, 1)
	PQ(2, 2)
	PQ(3, 0)
	PQ(3, 1)
	
	t.d[0] = p00;
	three_sum_3(p01, p10, q00, &t.d[1], &oe2, &oe3_0);
	six_three_sum(p02, p20, p11, q10, q01, oe2, &t.d[2], &oe3_1, &oe4_0); 
	nine_two_sum(p03, p30, p12, p21, q02, q20, q11, oe3_0, oe3_1, &t.d[3], &oe4_1);
	nine_two_sum(p31, p13, p22, q21, q12, q30, q03, oe4_0, oe4_1, &t.d[4], &oe4_1);// last term is ignored
	
	renormalize(&t, prod);
}
















