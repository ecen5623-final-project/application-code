INCLUDE_DIRS = -Iinclude
LIB_DIRS     =
CC           = g++

CDEFS        =
CFLAGS       = -O0 -g -Wall $(INCLUDE_DIRS) $(CDEFS) `pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv`
LIBS         = -lrt -lpthread
CPPLIBS      = `pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv`

SRCS = src/main.cpp \
       src/realtime.cpp \
       src/camera_buffer.cpp \
       src/camera_thread.cpp \
       src/display_buffer.cpp \
       src/hsv_config.cpp \
       src/sequencer.cpp

OBJS = $(SRCS:.cpp=.o)

all: capture

capture: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(CPPLIBS) $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o capture

.PHONY: all clean
