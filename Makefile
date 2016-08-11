OBJS = objs

CFLAGS += -I/usr/local/include/opencv -I/usr/local/include -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DPI
LDFLAGS += -L/usr/local/lib -lopencv_highgui -lopencv_core -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lpthread -lm -L/usr/lib/arm-linux-gnueabihf -lSDL
CFLAGS += -O3 -mfpu=neon
LDFLAGS += -lraspicamcv -lm -lrt -lwiringPi


all: pi_photobooth

%o: %c
	gcc -c $(CFLAGS) $< -o $@

pi_photobooth : pi_photobooth.o
	gcc $(LDFLAGS) -o $@ pi_photobooth.o

clean:
	rm -f *.o
	rm pi_photobooth
