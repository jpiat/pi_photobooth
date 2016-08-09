#include "opencv2/highgui/highgui_c.h"
#include "opencv2/core/core_c.h"
#include "opencv2/core/types_c.h"
#include "opencv2/imgproc/imgproc_c.h"
#include <SDL.h>
#include <SDL_video.h>

unsigned int PREVIEW_WIDTH = 1366;
unsigned int PREVIEW_HEIGHT = 768;
SDL_Surface* gScreenSurface = NULL, *gWindow;

int init_sdl() {

	int success = 1;
	if (SDL_Init( SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = 0;
	} else {
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED, PREVIEW_WIDTH, PREVIEW_HEIGHT,
				SDL_WINDOW_FULLSCREEN | SDL_WINDOW_OPENGL);
		if (gWindow == NULL) {
			printf("Window could not be created! SDL_Error: %s\n",
					SDL_GetError());
			success = 0;
		} else {
			gScreenSurface = SDL_GetWindowSurface(gWindow);
		}
	}
	return success;
}

SDL_Surface * ipl_to_sdl(IplImage * img) {
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom((void*) img->imageData,
			img->width, img->height, img->depth * img->nChannels,
			img->widthStep, 0xff0000, 0x00ff00, 0x0000ff, 0);
	return surface;
}

inline void bgr_to_hsv(char * bgr_pix, float * h, float * s, float * v) {
	unsigned char r, g, b;
	float c, M, m;
	b = bgr_pix[0];
	g = bgr_pix[1];
	r = bgr_pix[2];
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

}

void learn_background(CvCapture * capture, double * h_mean, double * s_mean,
		double * h_stdev, double * s_stdev, int nb_images) {
	int x, y;
	int i;
	IplImage * origin;
	IplImage * preview = cvCreateImage(cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
	IPL_DEPTH_8U, 3);
	memset(h_mean, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(s_mean, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(h_stdev, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	memset(s_stdev, 0, PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	for (i = 0; i < nb_images; i++) {
		float h, s, v;
		origin = cvQueryFrame(capture);
		cvResize(origin, preview, CV_INTER_CUBIC);
		for (y = 0; y < preview->height; y++) {
			for (x = 0; x < preview->width; x++) {
				bgr_to_hsv(
						&(preview->imageData[(y * preview->widthStep)
								+ (x * preview->nChannels)]), &h, &s, &v);
				h_mean[(y * preview->width) + x] += (double) h;
				s_mean[(y * preview->width) + x] += (double )s;
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
		origin = cvQueryFrame(capture);
		cvResize(origin, preview, CV_INTER_CUBIC);
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
	int x, y;
	CvCapture * capture;
	IplImage * origin;

	IplImage * background_image = cvLoadImage(
			"/home/jpiat/Pictures/background_filou.jpg", CV_LOAD_IMAGE_COLOR);
	IplImage * preview = cvCreateImage(cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
	IPL_DEPTH_8U, 3);

	IplImage * background_mask = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
			IPL_DEPTH_8U, 1);
	IplImage * preview_background = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT), IPL_DEPTH_8U, 3);
	IplImage * background_learnt = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT), IPL_DEPTH_8U, 1);
	double * h_mean, *s_mean, *h_stdev, *s_stdev;

	h_mean = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	s_mean = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	h_stdev = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	s_stdev = malloc(PREVIEW_WIDTH * PREVIEW_HEIGHT * sizeof(double));
	if (h_mean == NULL || s_mean == NULL || h_stdev == NULL || s_stdev == NULL) {
		printf("Allocation error \n");
		exit(-1);
	}
	cvResize(background_image, preview_background, CV_INTER_CUBIC);
	capture = cvCaptureFromCAM(0);
	//init_sdl();
	printf("Learning background \n");
	learn_background(capture, h_mean, s_mean, h_stdev, s_stdev, 100);
	printf("Background learnt \n");
	for (y = 0; y < background_learnt->height; y++) {
		for (x = 0; x < background_learnt->width; x++) {
			double h_pos = h_mean[x+(y*PREVIEW_WIDTH)] ;
			unsigned char h_u8 = (unsigned char) h_pos ;
			background_learnt->imageData[(y * background_learnt->widthStep) + x] =
					h_u8;
		}
	}
	cvShowImage("back", background_learnt);
	while (1) {
		origin = cvQueryFrame(capture);
		cvResize(origin, preview, CV_INTER_CUBIC);
		for (y = 0; y < preview->height; y++) {
			for (x = 0; x < preview->width; x++) {
				float h, s, v;
				double dist_h, dist_s, ch_std, cs_std, h_mean_pix, s_mean_pix;
				bgr_to_hsv(
						&(preview->imageData[(y * preview->widthStep)
								+ (x * preview->nChannels)]), &h, &s, &v);
				background_mask->imageData[y * background_mask->widthStep
						+ (x * background_mask->nChannels)] = 0x00;

				//Compute distance to background mean
				h_mean_pix = h_mean[y * preview->width + x];
				s_mean_pix = s_mean[y * preview->width + x];
				dist_h = fabs(h_mean_pix - h);
				dist_s = fabs(s_mean_pix - s);
				ch_std = h_stdev[y * preview->width + x];
				cs_std = s_stdev[y * preview->width + x];
				if (dist_h <= (2.5*ch_std) && dist_s <= (2.5* cs_std)) {
					background_mask->imageData[y * background_mask->widthStep
							+ (x * background_mask->nChannels)] = 0xFF;
				}
			}
		}

		for (y = 1; y < (preview->height - 1); y++) {
			for (x = 1; x < (preview->width - 1); x++) {
				unsigned char is_background = 0;
				//dilate
				is_background |= background_mask->imageData[y
						* background_mask->widthStep
						+ (x * background_mask->nChannels)];
				is_background |= background_mask->imageData[(y + 1)
						* background_mask->widthStep
						+ (x * background_mask->nChannels)];
				is_background |= background_mask->imageData[(y - 1)
						* background_mask->widthStep
						+ (x * background_mask->nChannels)];
				is_background |= background_mask->imageData[y
						* background_mask->widthStep
						+ ((x + 1) * background_mask->nChannels)];
				is_background |= background_mask->imageData[y
						* background_mask->widthStep
						+ ((x - 1) * background_mask->nChannels)];

				if (is_background != 0) {
					preview->imageData[y * preview->widthStep
							+ (x * preview->nChannels)] =
							preview_background->imageData[y
									* preview_background->widthStep
									+ (x * preview_background->nChannels)];
					preview->imageData[y * preview->widthStep
							+ (x * preview->nChannels) + 1] =
							preview_background->imageData[y
									* preview_background->widthStep
									+ (x * preview_background->nChannels) + 1];
					preview->imageData[y * preview->widthStep
							+ (x * preview->nChannels) + 2] =
							preview_background->imageData[y
									* preview_background->widthStep
									+ (x * preview_background->nChannels) + 2];
				}
			}
		}

		/*SDL_Surface * sdl_surface = ipl_to_sdl(preview);
		 SDL_BlitSurface(sdl_surface, NULL, gScreenSurface, NULL);
		 SDL_UpdateWindowSurface(gWindow);
		 SDL_Event event;
		 if (SDL_PollEvent(&event)== 1) {
		 switch (event.type) {
		 case SDL_KEYDOWN:
		 exit(0);
		 default:
		 break;
		 }
		 }
		 SDL_Delay(10);
		 SDL_FreeSurface(sdl_surface);*/

		cvShowImage("preview", preview);
		cvWaitKey(1);
	}
}
