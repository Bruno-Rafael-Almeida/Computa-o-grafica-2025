CC = g++
CFLAGS = -Wall -std=c++17
GLLIBS = -lglut -lGLEW -lGL -lGLU
ASSIMPLIBS = -lassimp

TARGET = mesh2
SRC = mesh2.cpp

all: $(TARGET)

mesh2: mesh2.cpp
	$(CC) $(CFLAGS) mesh2.cpp -o mesh2 $(GLLIBS) $(ASSIMPLIBS)

clean:
	rm -f mesh2
