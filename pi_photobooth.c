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
#define STD_DEV_TOLERANCE_H 0.8
#define STD_DEV_TOLERANCE_S 3.5

int display_width;
int display_height; //#define SDL2
char * output_path = NULL;
int timeout = -1 ;
CvFont font ;
#ifdef PI
RaspiCamCvCapture * capture;
#define TRIGGER_PIN 17
#else
CvCapture * capture;
#endif

#ifdef PI
SDL_Surface* gScreenSurface = NULL, *gWindow;

#ifdef SDL2

int init_sdl() {

	int success = 1;
	if (SDL_Init( SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		success = 0;
	} else {
		gWindow = SDL_CreateWindow("SDL Tutorial", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, PREVIEW_WIDTH, PREVIEW_HEIGHT,
				SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
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

#else
int init_sdl() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		SDL_Quit();
		return 0;
	}

	const SDL_VideoInfo* videoInfo = SDL_GetVideoInfo ();
	int systemX = videoInfo->current_w;
	int systemY = videoInfo->current_h;
	int systemZ = videoInfo->vfmt->BitsPerPixel;
	display_width = systemX;
	display_height = systemY;
	printf ("%d x %d, %d bpp\n", systemX, systemY, systemZ);
	//SDL_Quit();
	//exit(0);
	SDL_ShowCursor(0);
	gScreenSurface = SDL_SetVideoMode(systemX, systemY, systemZ,
			SDL_HWSURFACE | SDL_FULLSCREEN | SDL_DOUBLEBUF);
	if (gScreenSurface == NULL) {
		SDL_Quit();
		printf("SDL_SetVideoMode failed: %s\n");
		return 0;
	}
	return 1;
}
SDL_Surface * ipl_to_sdl(IplImage * img) {
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom((void*) img->imageData,
			img->width, img->height, img->depth * img->nChannels,
			img->widthStep, 0xff0000, 0x00ff00, 0x0000ff, 0);
	return surface;
}

#endif

#endif

struct timespec start_time;
struct timespec end_time;
struct timespec diff_time;

int timespec_subtract(struct timespec * result, struct timespec * x,
		struct timespec * y) {
	/* Perform the carry for the later subtraction by updating y. */
	if (x->tv_nsec < y->tv_nsec) {
		double nbsec = (y->tv_nsec - x->tv_nsec) / 1000000000.0;
		y->tv_nsec -= nbsec * 1000000000;
		y->tv_sec += nbsec;
	}
	if (x->tv_nsec - y->tv_nsec > 1000000000) {
		double nbsec = (x->tv_nsec - y->tv_nsec) / 1000000000.0;
		y->tv_nsec += nbsec * 1000000000.0;
		y->tv_sec -= nbsec;
	}

	/* Compute the time remaining to wait.
	 tv_usec is certainly positive. */
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_nsec = x->tv_nsec - y->tv_nsec;

	/* Return 1 if result is negative. */
	return x->tv_sec < y->tv_sec;
}

inline int erode_pos(IplImage * img, int x, int y) {
	int eroded_pixel = 1;
	eroded_pixel &= img->imageData[y * img->widthStep + (x * img->nChannels)];
	eroded_pixel &= img->imageData[(y + 1) * img->widthStep
			+ (x * img->nChannels)];
	eroded_pixel &= img->imageData[(y - 1) * img->widthStep
			+ (x * img->nChannels)];
	eroded_pixel &= img->imageData[y * img->widthStep
			+ ((x + 1) * img->nChannels)];
	eroded_pixel &= img->imageData[y * img->widthStep
			+ ((x - 1) * img->nChannels)];
	return eroded_pixel;
}

inline int dilate_pos(IplImage * img, int x, int y) {
	int dilate_pixel = 0;
	dilate_pixel |= img->imageData[y * img->widthStep + (x * img->nChannels)];
	dilate_pixel |= img->imageData[(y + 1) * img->widthStep
			+ (x * img->nChannels)];
	dilate_pixel |= img->imageData[(y - 1) * img->widthStep
			+ (x * img->nChannels)];
	dilate_pixel |= img->imageData[y * img->widthStep
			+ ((x + 1) * img->nChannels)];
	dilate_pixel |= img->imageData[y * img->widthStep
			+ ((x - 1) * img->nChannels)];
	return dilate_pixel;
}

int main(int argc, char ** argv) {
	int x, y, i;
	int pin_state = 1;
	IplImage * origin;

	if (argc < 2) {
		printf("Specify the background image to use \n");
		return -1;
	}

	if (argc > 2)
		output_path = argv[2];

	IplImage * background_image = cvLoadImage(argv[1], CV_LOAD_IMAGE_COLOR);
#ifdef PI
	IplImage * preview;
#else
	IplImage * preview = cvCreateImage(cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
	IPL_DEPTH_8U, 3);
#endif

	IplImage * background_mask = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT),
			IPL_DEPTH_8U, 1);
	IplImage * preview_background = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT), IPL_DEPTH_8U, 3);
	IplImage * background_learnt = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT), IPL_DEPTH_8U, 1);

	IplImage * eroded_mask = cvCreateImage(
			cvSize(PREVIEW_WIDTH, PREVIEW_HEIGHT), IPL_DEPTH_8U, 1);
	cvInitFont(&font, CV_FONT_HERSHEY_SIMPLEX, 8.0, 8.0, 0, 8.0, 8);
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

#ifdef PI
	RASPIVID_CONFIG * config = (RASPIVID_CONFIG*)malloc(sizeof(RASPIVID_CONFIG));
	RASPIVID_PROPERTIES * properties = (RASPIVID_PROPERTIES*)malloc(sizeof(RASPIVID_PROPERTIES));
	config->width=PREVIEW_WIDTH;
	config->height=PREVIEW_HEIGHT;
	config->bitrate=0;      // zero: leave as default
	config->framerate=FRAMERATE;
	config->monochrome=0;
	properties->hflip = H_FLIP;
	properties->vflip =V_FLIP;
	properties -> sharpness = 0;
	properties -> contrast = 0;
	properties -> brightness = 50;
	properties -> saturation = 0;
	properties -> exposure = AUTO;
	properties -> shutter_speed = SHUTTER_SPEED;
	properties->awb=AWB;
	properties->awb_gr = AWB_R ;
	properties->awb_gb = AWB_B ; 
	printf("Init sensor \n");
	capture = (RaspiCamCvCapture *) raspiCamCvCreateCameraCapture3(0, config, properties, 1);
	free(config);
	printf("Wait stable sensor \n");
	for(i = 0; i < 10; i ++ ) {
		int success = 0;
		IplImage* image = raspiCamCvQueryFrame(capture);
	}
	//if(init_sdl() == 0)printf("Failed to start display\n");
	wiringPiSetupSys();
	/*pinMode (0, INPUT);//setup pin 0 to trigger capture
	pullUpDnControl(0, PUD_UP);*/
	pin_state = digitalRead(TRIGGER_PIN);
	printf("Current pin state is %d \n", pin_state);
#else
	capture = cvCaptureFromCAM(0);
#endif
	printf("Loading background model\n");

	FILE * fd_hmean = fopen("./background_hmean.raw", "rb");
	FILE * fd_hstdev = fopen("./background_hstdev.raw", "rb");
	FILE * fd_smean = fopen("./background_smean.raw", "rb");
	FILE * fd_sstdev = fopen("./background_sstdev.raw", "rb");

	if (fd_hmean == NULL || fd_hstdev == NULL || fd_smean == NULL
			|| fd_sstdev == NULL) {
		printf("No calibration file found \n");
		exit(0);
	}

	//should check return to make sure all data was read
	fread(h_mean, sizeof(double), PREVIEW_WIDTH * PREVIEW_HEIGHT, fd_hmean);
	fread(h_stdev, sizeof(double), PREVIEW_WIDTH * PREVIEW_HEIGHT, fd_hstdev);
	fread(s_mean, sizeof(double), PREVIEW_WIDTH * PREVIEW_HEIGHT, fd_smean);
	fread(s_stdev, sizeof(double), PREVIEW_WIDTH * PREVIEW_HEIGHT, fd_sstdev);

	fclose(fd_hmean);
	fclose(fd_hstdev);
	fclose(fd_smean);
	fclose(fd_sstdev);

	printf("Background loaded \n");
	for (y = 0; y < background_learnt->height; y++) {
		for (x = 0; x < background_learnt->width; x++) {
#ifdef FAST_CONV
			double h_pos = h_mean[x + (y * PREVIEW_WIDTH)] * 255;
			unsigned char h_u8 = (unsigned char) h_pos;
#else
			double h_pos = h_mean[x + (y * PREVIEW_WIDTH)];
			unsigned char h_u8 = (unsigned char) h_pos;
#endif

			background_learnt->imageData[(y * background_learnt->widthStep) + x] =
					h_u8;
		}
	}
#ifdef PI
	if (init_sdl() == 0) {
		printf("Failed to init display \n");
		SDL_Quit();
		exit(-1);
	}
#endif
	//cvShowImage("back", background_learnt);
	while (1) {
#ifdef PI
		preview = raspiCamCvQueryFrame(capture); // Pi capture at the preview size
#else
		origin = cvQueryFrame(capture); //webcam may not support resolution
		cvResize(origin, preview, CV_INTER_CUBIC);
#endif
		clock_gettime(CLOCK_REALTIME, &start_time);
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
				if (dist_h <= (STD_DEV_TOLERANCE_H * ch_std)
						/*&& dist_s <= (STD_DEV_TOLERANCE_S * cs_std)*/) {
					background_mask->imageData[y * background_mask->widthStep
							+ (x * background_mask->nChannels)] = 0xFF;
				}

				//need to perform dilate of background ...
				int decal_x, decal_y;
				if (x > 1 && y > 1) {
					//new pixel coordinate for shifted 3x3 window
					decal_x = x - 1;
					decal_y = y - 1;

					eroded_mask->imageData[decal_y * eroded_mask->widthStep
							+ ((decal_x + 1) * eroded_mask->nChannels)] =
							dilate_pos(background_mask, decal_x, decal_y);

					//now that we eroded the background, we can dilate
					if (x > 2 && y > 2) {
						//we shift the current kernel to the left
						decal_x = x - 2;
						decal_y = y - 2;
						if (erode_pos(eroded_mask, decal_x, decal_y) != 0) {
							preview->imageData[decal_y * preview->widthStep
									+ (decal_x * preview->nChannels)] =
									preview_background->imageData[decal_y
											* preview_background->widthStep
											+ (decal_x
													* preview_background->nChannels)];
							preview->imageData[decal_y * preview->widthStep
									+ (decal_x * preview->nChannels) + 1] =
									preview_background->imageData[decal_y
											* preview_background->widthStep
											+ (decal_x
													* preview_background->nChannels)
											+ 1];
							preview->imageData[decal_y * preview->widthStep
									+ (decal_x * preview->nChannels) + 2] =
									preview_background->imageData[decal_y
											* preview_background->widthStep
											+ (decal_x
													* preview_background->nChannels)
											+ 2];
						}
					}

				}
			}
		}
		clock_gettime(CLOCK_REALTIME, &end_time);
		timespec_subtract(&diff_time, &end_time, &start_time);
		printf("%s :  %lu s, %lu ns \n", "processing : ", diff_time.tv_sec,
				diff_time.tv_nsec);
		if(timeout > 5){
			char buff [10] ;
			sprintf(buff, "%d", (timeout/10)+1);
			cvPutText(preview, buff, cvPoint((preview->width/2)-100, preview->height/2), &font,cvScalar(255, 255, 255, 0));
		}
#ifdef PI
		SDL_Surface * sdl_surface = ipl_to_sdl(preview);
		SDL_Rect position;
		position.x = (display_width/2) - (PREVIEW_WIDTH/2);
		position.y = 0;
		SDL_BlitSurface(sdl_surface, NULL, gScreenSurface, &position);
#ifdef SDL2
		SDL_UpdateWindowSurface(gWindow);
#else
		SDL_UpdateRect(gScreenSurface, 0, 0, 0, 0);
		SDL_Flip(gScreenSurface);
#endif
		SDL_Event event;
		if (SDL_PollEvent(&event)== 1) {
			switch (event.type) {
				case SDL_KEYDOWN:
				if(output_path != NULL) cvSaveImage(output_path, preview, 0);
				raspiCamCvReleaseCapture(&capture);
				SDL_Quit();
				exit(0);
				default:
				break;
			}
		}
		SDL_Delay(1);
		SDL_FreeSurface(sdl_surface);
		if(timeout < 0 && digitalRead(TRIGGER_PIN) == 1 && pin_state == 0 ) { //Rising edge ends program
			//Should record the last fused frame for preview purpose
			timeout = 30 ;
			/*SDL_Quit();
			exit(0);*/
		}
		pin_state = digitalRead(TRIGGER_PIN);
		if(timeout == 0){
			if(output_path != NULL) cvSaveImage(output_path, preview, 0);
			raspiCamCvReleaseCapture(&capture);
			SDL_Quit();
			exit(0);
		}
		if(timeout > 0) timeout -- ;
#else
		cvShowImage("preview", preview);
		cvWaitKey(1);
#endif

	}
#ifdef PI
	raspiCamCvReleaseCapture(&capture);
	SDL_Quit();
#endif
}
