INCLUDE_DIRS = -Iinclude -I.src/Config -I.
LIB_DIRS =
CC = gcc
CXX = g++

CDEFS = -DUSE_BCM2835_LIB
CFLAGS = -O0 -g -Wall $(INCLUDE_DIRS) $(CDEFS) `pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv`
LIBS = -lrt -lpthread -lbcm2835 -lm
CPPLIBS = `pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv`

SRCS = src/main.cpp \
       src/realtime.cpp \
       src/camera_buffer.cpp \
       src/camera_thread.cpp \
       src/display_buffer.cpp \
       src/hsv_config.cpp \
       src/sequencer.cpp \
	   src/LCD_2inch.c \
	   src/Config/DEV_Config.c

OBJS = $(SRCS:.cpp=.o)

all: capture

capture: $(OBJS)
	$(CXX) $(CFLAGS) -o $@ $(OBJS) $(CPPLIBS) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o capture

.PHONY: all clean
