#ifndef __sti__misc_h__
#define __sti__misc_h__


// *performance* time counting functions

// these use CLOCK_MONOTONIC_RAW (>= Linux 2.6.28)
// they do not provide a real unix timestamp, rather a consistent, precise measure of
//   some undefined time that is unaffected by system time twiddling
double getCurrentTimePerf(); // in seconds
double timeSincePerf(double past); // also in seconds


// *actual wall time* functions 

// these give unix timestamps
// this function provides the number of seconds since the unix epoch
double getCurrentTimeEpoch(); // in seconds
// deceptively but consistently named, this function is comparative with stamps
//   provided by the previous function
double timeSinceEpoch(double past); // also in seconds




// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static unsigned long inline nextPOT(unsigned long in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}



#endif // __sti__misc_h__
