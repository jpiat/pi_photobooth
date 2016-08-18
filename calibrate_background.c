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


float pop_h[360] ;
float pop_s[360] ;
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
	memset(pop_h, 0, sizeof(float)*360);
	memset(pop_s, 0, sizeof(float)*360);
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
				pop_h[(unsigned int) h] +=1.0 ;
				pop_s[(unsigned int) h] += (s/nb_images) ;
				h_mean[(y * preview->width) + x] += ((double) h)/nb_images;
				s_mean[(y * preview->width) + x] += ((double) s)/nb_images;
			}
		}
	}
	/*for (y = 0; y < preview->height; y++) {
		for (x = 0; x < preview->width; x++) {
			h_mean[(y * preview->width) + x] /= nb_images;
			s_mean[(y * preview->width) + x] /= nb_images;
		}
	}*/
/*	for (i = 0; i < nb_images; i++) {
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
				h_stdev[(y * preview->width) + x] += (dist_h/nb_images);
				s_stdev[(y * preview->width) + x] += (dist_s/nb_images);
			}
		}
	}
*/
	/*for (y = 0; y < preview->height; y++) {
		for (x = 0; x < preview->width; x++) {
			h_stdev[(y * preview->width) + x] /= nb_images;
			s_stdev[(y * preview->width) + x] /= nb_images;
		}
	}*/
	unsigned int index_max, index_dev_min, index_dev_max ;
	float pop_max = 0. ;
	for(i = 0 ; i < 360 ; i ++){
		if(pop_h[i] > pop_max){
			pop_max = pop_h[i];
			index_max = i ;
		}
		//printf("%f ", pop_h[i]);
	}
	printf("Max pop is : %d \n", index_max);
	for(i = index_max ; i < 360 ; i ++){
		if(pop_h[i] == 0.0){
			index_dev_max = (i-1) ;
		 	break ;
		}
	}
	for(i = index_max ; i >= 0 ; i  --){
		if(pop_h[i] == 0.0){
			index_dev_min = (i+1) ;
			break ;
		}
	}
	printf("Median is : %d \n", (index_dev_min+index_dev_max)/2);
	printf("Max dev is : %d \n", (index_dev_max - index_dev_min)/2);
	float pop_s_mean = 0. ;
	for(i = index_dev_min ; i <= index_dev_max ; i ++){
		pop_s_mean += pop_s[i];
		printf("%f ", pop_s[i]);
	}
	pop_s_mean /= ((index_dev_max - index_dev_min) + 1) ;
	printf("Saturation mean for H : %f \n", pop_s_mean);
	for(i = 0 ; i < (preview->height * preview->width); i ++){
		h_mean[i] = (index_dev_min+index_dev_max)/2.;
		h_stdev[i] = (index_dev_max - index_dev_min)/2. ;
		s_mean[i] = pop_s_mean ;
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
	for(i = 0; i < 100; i ++ ) {
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
!	fwrite(s_stdev, sizeof(double), PREVIEW_WIDTH*PREVIEW_HEIGHT, fd_sstdev);
	fclose(fd_sstdev);
}
