#ifndef __sti__stats_h__
#define __sti__stats_h__



typedef struct sti_stats {
	double sum;
	double mean;
	double min;
	size_t min_index;
	double max;
	size_t max_index;
	double median;
	size_t median_index;
	
	double pop_var;
	double std_dev;
	
} sti_stats;

int calc_stats_f(void* data, size_t stride, ptrdiff_t val_offset, size_t len, sti_stats* st);
int calc_stats_d(void* data, size_t stride, ptrdiff_t val_offset, size_t len, sti_stats* st);


#endif //__sti__stats_h__
