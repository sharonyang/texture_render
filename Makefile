CC = g++
FLAGS = -g -o

INCLUDE = -I/home/sharon/Downloads/hw5/basecode -I/usr/X11R6/include -I/usr/include/GL -I/usr/include
LIBDIR = -L/usr/X11R6/lib -L/usr/local/lib
SOURCES = *.cpp *.h
LIBS = -lGLEW -lGL -lGLU -lglut -lm

EXENAME = texture

all: $(SOURCES)
	$(CC) $(FLAGS) $(EXENAME) $(INCLUDE) $(LIBDIR) $(SOURCES) $(LIBS)

clean:
	rm -f *.o $(EXENAME)

.PHONY: all clean
