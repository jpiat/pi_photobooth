OBJS = objs

CFLAGS += -I/usr/local/include/opencv -I/usr/local/include
LDFLAGS += -L/usr/local/lib -lopencv_highgui -lopencv_core -lopencv_video -lopencv_features2d -lopencv_calib3d -lopencv_imgproc -lpthread -lm
CFLAGS += -O3 -mfpu=neon
LDFLAGS += -lraspicamcv -lm -lrt -llz4


all: pi_photobooth

%o: %c
	gcc -c $(CFLAGS) $< -o $@

pi_photobooth : $(OBJS) pi_photobooth.o
	gcc $(LDFLAGS) -o $@ pi_photobooth.o $(OBJS)

clean:
	rm -f *.o
	rm pi_photobooth
