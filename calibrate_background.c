#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "opencv2/highgui/highgui_c.h"
#include "opencv2/core/core_c.h"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/imgproc_c.h"

#ifdef PI
#include "RaspiCamCV.h"
#include <SDL.h>
#include <SDL_video.h>
#include <wiringPi.h>
#endif

#include "camera_config.h"

int display_width ;
int display_height ;

#ifdef PI
RaspiCamCvCapture * capture;
#else
CvCapture * capture;
#endif

inline void bgr_to_hsv(char * bgr_pix, float * h, float * s, float * v) {

#ifndef FAST_CONV
	float r, g, b;
	float c, M, m;
	b = ((float) ((unsigned char*) bgr_pix)[0])/255.O;
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
	b = (float) ((unsigned char*) bgr_pix)[0];
	g = (float) ((unsigned char*) bgr_pix)[1];
	r = (float) ((unsigned char*) bgr_pix)[2];
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
	(*s) = chroma / (r + 1e-20f);
	(*v) = r;
#endif
}

void learn_background(double * h_mean, double * s_mean, double * h_stdev,
		double * s_stdev, int nb_images) {
	int x, y;
	int i;
	IplImage * origin;
#ifdef PI
	IplImage * preview;
#else
	IplImage * preview = cvCreateImage(cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
	IPL_DEPTH_8U, 3);
#endif

	memset(h_mean, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(s_mean, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(h_stdev, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(s_stdev, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	for (i = 0; i < nb_images; i++) {
		float h, s, v;
#ifdef PI
		preview = raspiCamCvQueryFrame(capture);
#else
		origin = cvQueryFrame(capture);
		cvResize(origin, preview, CV_INTER_CUBIC);
#endif

		for (y = 0; y < preview->height; y++) {
			for (x = 0; x < preview->width; x++) {
				bgr_to_hsv(
						&(preview->imageData[(y * preview->widthStep)
								+ (x * preview->nChannels)]), &h, &s, &v);
				h_mean[(y * preview->width) + x] += (double) h;
				s_mean[(y * preview->width) + x] += (double) s;
			}
		}
	}
	for (y = 0; y < preview->height; y++) {
		for (x = 0; x < preview->width; x++) {
			h_mean[(y * preview->width) + x] /= nb_images;
			s_mean[(y * preview->width) + x] /= nb_images;
		}
	}
	for (i = 0; i < nb_images; i++) {
		float h, s, v;
#ifdef PI
		preview = raspiCamCvQueryFrame(capture);
#else
		origin = cvQueryFrame(capture);
		cvResize(origin, preview, CV_INTER_CUBIC);
#endif

		for (y = 0; y < preview->height; y++) {
			for (x = 0; x < preview->width; x++) {
				double dist_h, dist_s;
				bgr_to_hsv(
						&(preview->imageData[(y * preview->widthStep)
								+ (x * preview->nChannels)]), &h, &s, &v);
				dist_h = fabs(h - h_mean[y * preview->width + x]);
				dist_s = fabs(s - s_mean[y * preview->width + x]);
				h_stdev[(y * preview->width) + x] += dist_h;
				s_stdev[(y * preview->width) + x] += dist_s;
			}
		}
	}

	for (y = 0; y < preview->height; y++) {
		for (x = 0; x < preview->width; x++) {
			h_stdev[(y * preview->width) + x] /= nb_images;
			s_stdev[(y * preview->width) + x] /= nb_images;
		}
	}

}

int main(int argc, char ** argv) {
	int x, y, i;
	int pin_state = 1 ;
	IplImage * origin;

#ifdef PI
	IplImage * preview;
#else
	IplImage * preview = cvCreateImage(cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
	IPL_DEPTH_8U, 3);
#endif

	double * h_mean, *s_mean, *h_stdev, *s_stdev;

	h_mean = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	s_mean = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	h_stdev = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	s_stdev = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	if (h_mean == NULL || s_mean == NULL || h_stdev == NULL || s_stdev == NULL) {
		printf("Allocation error \n");
		exit(-1);
	}
	

#ifdef PI
	RASPIVID_CONFIG * config = (RASPIVID_CONFIG*)malloc(sizeof(RASPIVID_CONFIG));
	RASPIVID_PROPERTIES * properties = (RASPIVID_PROPERTIES*)malloc(sizeof(RASPIVID_PROPERTIES));
	config->width=PREVIEW_WIDTH;
	config->height=PREVIEW_HEIGHT;
	config->bitrate=0;      // zero: leave as default
	config->framerate=FRAMERATE;
	config->monochrome=0;
	properties->hflip = H_FLIP;
	properties->vflip = V_FLIP;
	properties -> sharpness = 0;
	properties -> contrast = 0;
	properties -> brightness = 50;
	properties -> saturation = 0;
	properties -> exposure = AUTO;
	properties -> shutter_speed = SHUTTER_SPEED;
	properties->awb=AWB;
	properties->awb_gr=AWB_R;
	properties->awb_gb=AWB_B;
	printf("Init sensor \n");
	capture = (RaspiCamCvCapture *) raspiCamCvCreateCameraCapture3(0, config, properties, 1);
	free(config);
	printf("Wait stable sensor \n");
	for(i = 0; i < 10; i ++ ) {
		int success = 0;
		IplImage* image = raspiCamCvQueryFrame(capture);
	}
#else
	capture = cvCaptureFromCAM(0);
#endif
	printf("Learning background \n");
	learn_background(h_mean, s_mean, h_stdev, s_stdev, 100);
	
	FILE * fd_hmean = fopen("./background_hmean.raw", "wb");
	fwrite(h_mean, sizeof(double), PREVIEW_WIDTH*PREVIEW_HEIGHT, fd_hmean);
	fclose(fd_hmean);

	FILE * fd_hstdev = fopen("./background_hstdev.raw", "wb");
	fwrite(h_stdev, sizeof(double), PREVIEW_WIDTH*PREVIEW_HEIGHT, fd_hstdev);
	fclose(fd_hstdev);

	FILE * fd_smean = fopen("./background_smean.raw", "wb");
	fwrite(s_mean, sizeof(double), PREVIEW_WIDTH*PREVIEW_HEIGHT, fd_smean);
	fclose(fd_smean);

	FILE * fd_sstdev = fopen("./background_sstdev.raw", "wb");
	fwrite(s_stdev, sizeof(double), PREVIEW_WIDTH*PREVIEW_HEIGHT, fd_sstdev);
	fclose(fd_sstdev);
}
