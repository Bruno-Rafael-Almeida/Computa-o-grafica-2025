CC = g++
CFLAGS = -Wall -std=c++17
GLLIBS = -lglut -lGLEW -lGL -lGLU
ASSIMPLIBS = -lassimp

TARGET = mesh
SRC = mesh.cpp

all: $(TARGET)

mesh: mesh.cpp
	$(CC) $(CFLAGS) mesh.cpp -o mesh $(GLLIBS) $(ASSIMPLIBS)

clean:
	rm -f mesh
