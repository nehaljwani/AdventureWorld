CC = g++
CFLAGS = -Wall
PROG = Adventure 

SRCS = Adventure.cpp
LIBS = -lglut -lGL -lGLU #-ljpeg -lm

all: $(PROG)

$(PROG):	$(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
