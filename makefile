CC = g++
CFLAGS = -Wall -std=c++11 -pthread $(shell pkg-config --cflags opencv4)
LIBS = $(shell pkg-config --libs opencv4)

all: sobel_filter_threaded

sobel_filter_threaded: sobel_filter_threaded.cpp
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f sobel_filter_threaded