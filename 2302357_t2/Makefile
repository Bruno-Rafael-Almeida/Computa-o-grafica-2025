CC = g++
CFLAGS = -Wall -std=c++17
GLLIBS = -lglut -lGLEW -lGL -lGLU
ASSIMPLIBS = -lassimp

TARGET = mesh2_
SRC = mesh2_.cpp

all: $(TARGET)

mesh2_: mesh2_.cpp
	$(CC) $(CFLAGS) mesh2_.cpp -o mesh2_ $(GLLIBS) $(ASSIMPLIBS)

clean:
	rm -f mesh2_
