CC = g++

GLLIBS = -lglut -lGLEW -lGL

all: ex2.cpp ex3.cpp
	$(CC) ex2.cpp ../lib/utils.cpp -o ex2 $(GLLIBS)
	$(CC) ex3.cpp ../lib/utils.cpp -o ex3 $(GLLIBS)

clean:
	rm -f ex2 ex3
