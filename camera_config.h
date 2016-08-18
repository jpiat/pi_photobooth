#include <math.h>
unsigned int PREVIEW_WIDTH = 960;
unsigned int PREVIEW_HEIGHT = 720;
unsigned int FRAMERATE = 10 ;
unsigned int SHUTTER_SPEED = 20000;
unsigned int AWB=0;
float AWB_B = 1.6;
float AWB_R = 1.3;


#define H_FLIP 0
#define V_FLIP 0

#define FAST_CONV

inline void bgr_to_hsv(char * bgr_pix, float * h, float * s, float * v) {


#ifndef FAST_CONV
	float r, g, b;
	float c, M, m;
	b = ((float) ((unsigned char*) bgr_pix)[0])/255.0;
	g = ((float)  ((unsigned char*) bgr_pix)[1])/255.0;
	r = ((float) ((unsigned char*) bgr_pix)[2])/255.0;
	M = r > g ? r : g;
	M = b > M ? b : M;
	m = r < g ? r : g;
	m = b < m ? b : m;
	c = M - m;
	(*h) = 0;
	if (M == r)
	(*h) = fmod(((g - b) / c), 6);
	else if (M == g)
	(*h) = ((b - r) / c) + 2;
	else if (M == b)
	(*h) = ((r - g) / c) + 4;
	(*h) = (*h) * 60.0;
	(*v) = M;
	(*s) = (*v) > 0 ? (c / (*v)) : 0;

#else
	float r, g, b;
	b = (float) ((unsigned char*) bgr_pix)[0]/255.0;
	g = (float) ((unsigned char*) bgr_pix)[1]/255.0;
	r = (float) ((unsigned char*) bgr_pix)[2]/255.0;
	float K = 0.f;
	float chroma = 0.f;
	if (g < b) {
		float t;
		t = g;
		g = b;
		b = t;
		K = -1.f;
	}

	if (r < g) {
		float t;
		t = r;
		r = g;
		g = t;
		K = -2.f / 6.f - K;
	}

	if (g > b)
		chroma = r - b;
	else
		chroma = r - g;

	(*h) = fabs(K + (g - b) / (6.f * chroma + 1e-20f));
	(*h) = (*h) * 360.0 ;
	(*s) = chroma / (r + 1e-20f);
	(*v) = r;
#endif
}


