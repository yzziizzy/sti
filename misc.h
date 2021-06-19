#ifndef __sti__misc_h__
#define __sti__misc_h__

// Public Domain.

#include <stdlib.h>
#include <math.h> // fmin,fmax


// *performance* time counting functions

// these use CLOCK_MONOTONIC_RAW (>= Linux 2.6.28)
// they do not provide a real unix timestamp, rather a consistent, precise measure of
//   some undefined time that is unaffected by system time twiddling
double getCurrentTimePerf(void); // in seconds
double timeSincePerf(double past); // also in seconds


// *actual wall time* functions 

// these give unix timestamps
// this function provides the number of seconds since the unix epoch
double getCurrentTimeEpoch(void); // in seconds
// deceptively but consistently named, this function is comparative with stamps
//   provided by the previous function
double timeSinceEpoch(double past); // also in seconds




// super nifty site:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
inline static size_t nextPOT(size_t in) {
	
	in--;
	in |= in >> 1;
	in |= in >> 2;
	in |= in >> 4;
	in |= in >> 8;
	in |= in >> 16;
	in++;
	
	return in;
}


#ifndef STI_C3DLAS_NO_CONFLICT
// Random number helpers

inline static float frand(float low, float high) {
	return low + ((high - low) * ((float)rand() / (float)RAND_MAX));
}

inline static float frandNorm() {
	return ((float)rand() / (float)RAND_MAX);
}

inline static double drand(double low, double high) {
	return low + ((high - low) * ((double)rand() / (double)RAND_MAX));
}

inline static double drandNorm() {
	return ((double)rand() / (double)RAND_MAX);
}


// clamping

inline static float fclamp(float val, float min, float max) {
	return fminf(max, fmaxf(min, val));
}
inline static float fclampNorm(float val) {
	return fclamp(val, 0.0f, 1.0f);
}
inline static float dclamp(double val, double min, double max) {
	return fmin(max, fmax(min, val));
}
inline static float dclampNorm(double val) {
	return fclamp(val, 0.0, 1.0);
}

inline static int iclamp(int val, int min, int max) {
	return val > max ? max : (val < min ? min : val);
}
inline static int uiclamp(unsigned int val, unsigned int min, unsigned int max) {
	return val > max ? max : (val < min ? min : val);
}
inline static int iclamp64(int64_t val, int64_t min, int64_t max) {
	return val > max ? max : (val < min ? min : val);
}
inline static int uclamp64(uint64_t val, uint64_t min, uint64_t max) {
	return val > max ? max : (val < min ? min : val);
}


// lerps (linear interpolation)

inline static float flerp(float a, float b, float t) {
	return a  + ((b - a) * t);
}
inline static float dlerp(double a, double b, double t) {
	return a  + ((b - a) * t);
}
inline static float flerp2D(float x0y0, float x1y0, float x0y1, float x1y1, float tx, float ty) {
	return flerp(flerp(x0y0, x1y0, tx), flerp(x0y1, x1y1, tx), ty);
}
inline static float dlerp2D(double x0y0, double x1y0, double x0y1, double x1y1, double tx, double ty) {
	return dlerp(dlerp(x0y0, x1y0, tx), dlerp(x0y1, x1y1, tx), ty);
}

#endif



#endif // __sti__misc_h__
