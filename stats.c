
#include <math.h>




static int float_cmp(const void* a, const void* b) {
	float* c = *(float*)a;
	float* d = *(float*)b;
	return c == d ? 0 : (c > d ? 1 : 0);
}

typedef struct {
	size_t index;
	float val;
} bucket_f;


int calc_stats_f(void* data, size_t stride, ptrdiff_t val_offset, size_t len, sti_stats* st) {
	
	if(len < 2) return 1;
	
	double dlen = len;
	double sum = 0;
	
	bucket_f* tmp = malloc(sizeof(*tmp) * len);
	
	for(size_t i = 0; i < len; i++) {
		tmp[i].index = i;
		tmp[i].val = *((float*)((char*)data + (stride * i) + val_offset));
		sum += tmp[i].val;
	}
	
	qsort(tmp, len, sizeof(*tmp), float_cmp);
	
	st->sum = sum;
	st->min = tmp[0].val;
	st->min_index = tmp[0].index;
	st->max = tmp[len-1].val;
	st->max_index = tmp[len-1].index;
	st->mean = sum / dlen;
	
	size_t mi = floor((dlen / 2.0) + 0.5);
	st->median_index = tmp[mi].index;
	st->median = tmp[mi].val;
	
	sum = 0;
	float mean = st->mean;
	for(size_t i = 0; i < len; i++) {
		float v = tmp[i].val - mean;
		v *= v;
		sum += v;
	}
	
	sum /= dlen;
	
	st->pop_var = sum;
	st->std_dev = sqrt(sum);
	
	free(tmp);
	
	return 0;
};  





static int double_cmp(const void* a, const void* b) {
	double* c = *(double*)a;
	double* d = *(double*)b;
	return c == d ? 0 : (c > d ? 1 : 0);
}

typedef struct {
	size_t index;
	double val;
} bucket_d;



int calc_stats_d(void* data, size_t stride, ptrdiff_t val_offset, size_t len, sti_stats* st) {
	
	if(len < 2) return 1;
	
	double dlen = len;
	double sum = 0;
	
	bucket_d* tmp = malloc(sizeof(*tmp) * len);
	
	for(size_t i = 0; i < len; i++) {
		tmp[i].index = i;
		tmp[i].val = *((double*)((char*)data + (stride * i) + val_offset));
		sum += tmp[i].val;
	}
	
	qsort(tmp, len, sizeof(*tmp), double_cmp);
	
	st->sum = sum;
	st->min = tmp[0].val;
	st->min_index = tmp[0].index;
	st->max = tmp[len-1].val;
	st->max_index = tmp[len-1].index;
	st->mean = sum / dlen;
	
	size_t mi = floor((dlen / 2.0) + 0.5);
	st->median_index = tmp[mi].index;
	st->median = tmp[mi].val;
	
	sum = 0;
	double mean = st->mean;
	for(size_t i = 0; i < len; i++) {
		double v = tmp[i].val - mean;
		v *= v;
		sum += v;
	}
	
	sum /= dlen;
	
	st->pop_var = sum;
	st->std_dev = sqrt(sum);
	
	free(tmp);
	
	return 0;
};  













