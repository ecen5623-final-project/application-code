INCLUDE_DIRS = -I./Config -I.
CC = gcc
CXX = g++

CDEFS = -DUSE_BCM2835_LIB
CFLAGS = -O0 -g $(INCLUDE_DIRS) $(CDEFS)
CXXFLAGS = -O0 -g $(INCLUDE_DIRS) $(CDEFS)

LIBS = -lrt -lbcm2835 -lm
CPPLIBS = `pkg-config --libs opencv`

all: capture

clean:
	-rm -f *.o *.d Config/*.o
	-rm -f capture

capture: capture.o Config/DEV_Config.o LCD_2inch.o
	$(CXX) $(CXXFLAGS) -o $@ capture.o Config/DEV_Config.o LCD_2inch.o $(CPPLIBS) $(LIBS)

capture.o: capture.cpp
	$(CXX) $(CXXFLAGS) -c capture.cpp

Config/DEV_Config.o: Config/DEV_Config.c
	$(CC) $(CFLAGS) -c Config/DEV_Config.c -o Config/DEV_Config.o

LCD_2inch.o: LCD_2inch.c
	$(CC) $(CFLAGS) -c LCD_2inch.c -o LCD_2inch.o