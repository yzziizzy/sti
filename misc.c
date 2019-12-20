

#include <time.h>


// *performance* time counting functions

// these use CLOCK_MONOTONIC_RAW (>= Linux 2.6.28)
// they do not provide a real unix timestamp, rather a consistent, precise measure of
//   some undefined time that is unaffected by system time twiddling
double getCurrentTimePerf() { // in seconds
	double now;
	struct timespec ts;
	static double offset = 0;
	
	// CLOCK_MONOTONIC_RAW in >= Linux 2.6.28.
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	
	now = (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
	if(offset == 0) offset = now;
	
	return now - offset;
}

double timeSincePerf(double past) {
	double now = getCurrentTimePerf();
	return now - past;
}




// these give unix timestamps
// this function provides the number of seconds since the unix epoch
double getCurrentTimeEpoch() { // in seconds
	double now;
	struct timespec ts;
	static double offset = 0;
	
	// CLOCK_MONOTONIC_RAW in >= Linux 2.6.28.
	clock_gettime(CLOCK_REALTIME, &ts);
	
	now = (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
	if(offset == 0) offset = now;
	
	return now - offset;
}
// deceptively but consistently named, this function is comparative with stamps
//   provided by the previous function
double timeSinceEpoch(double past) {
	double now = getCurrentTimeEpoch();
	return now - past;
}
