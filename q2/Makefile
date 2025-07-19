CC = g++

GLLIBS = -lglut -lGLEW -lGL

all: vetores.cpp ex6.cpp
	$(CC) vetores.cpp ../lib/utils.cpp -o vetores $(GLLIBS)
	$(CC) ex6.cpp ../lib/utils.cpp -o ex6 $(GLLIBS)

clean:
	rm -f vetores ex6
