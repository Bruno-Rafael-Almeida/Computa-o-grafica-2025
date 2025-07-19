CC = g++

GLLIBS = -lglut -lGLEW -lGL

all: transform.cpp transform2.cpp q2.cpp
	$(CC) transform.cpp ../lib/utils.cpp -o transform $(GLLIBS)
	$(CC) transform2.cpp ../lib/utils.cpp -o transform2 $(GLLIBS)
	$(CC) q2.cpp ../lib/utils.cpp -o q2 $(GLLIBS)

clean:
	rm -f transform transform2 q2
