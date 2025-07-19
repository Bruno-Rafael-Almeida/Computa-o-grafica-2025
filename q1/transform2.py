#!/usr/bin/env python3

## @file transform.py
#  Applies geometric transformations to a rectangle.
# 
# Iteratively rotates a rectangle using two modes acording to keyboard key:
# 1 for around the center of the object;
# 2 for around coordinate system origin.
#
# @author Ricardo Dutra da Silva


import sys
import ctypes
import numpy as np
import OpenGL.GL as gl
import OpenGL.GLUT as glut
sys.path.append('../lib/')
import utils as ut
from ctypes import c_void_p


## Window width.
win_width  = 800
## Window height.
win_height = 600

## Program variable.
program = None
## Vertex array object.
VAO = None
## Vertex buffer object.
VBO = None

## Rotation angle.
angle = 0.0
## Rotation increment.
angle_inc = 0.05
## Rotation mode.
mode = 1

## Vertex shader.
vertex_code = """
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 vColor;

uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(position, 1.0);
    vColor = color;
}
"""

## Fragment shader.
fragment_code = """
#version 330 core

in vec3 vColor;
out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0f);
} 
"""

## Drawing function.
#
# Draws primitive.
def display():

    gl.glClearColor(0.2, 0.3, 0.3, 1.0)
    gl.glClear(gl.GL_COLOR_BUFFER_BIT)

    gl.glUseProgram(program)
    gl.glBindVertexArray(VAO)

    # Translation. 
    T  = ut.matTranslate(0.5, -0.5, 0.0)
    # Rotation around z-axis.
    Rz = ut.matRotateZ(np.radians(angle))
    # Scale.
    S  = ut.matScale(0.3, 0.5, 0.0)

    if mode == 1:
        M = np.matmul(Rz,S)
        M = np.matmul(T,M)
    elif mode == 2:
        M = np.matmul(T,S)
        M = np.matmul(Rz,M)

    # Retrieve location of tranform variable in shader.
    loc = gl.glGetUniformLocation(program, "transform");
    # Send matrix to shader.
    gl.glUniformMatrix4fv(loc, 1, gl.GL_FALSE, M.transpose())

    gl.glDrawArrays(gl.GL_TRIANGLES, 0, 6)

    glut.glutSwapBuffers()


## Reshape function.
# 
# Called when window is resized.
#
# @param width New window width.
# @param height New window height.
def reshape(width,height):

    win_width = width
    win_height = height
    gl.glViewport(0, 0, width, height)
    glut.glutPostRedisplay()


## Keyboard function.
#
# Called to treat pressed keys.
#
# @param key Pressed key.
# @param x Mouse x coordinate when key pressed.
# @param y Mouse y coordinate when key pressed.
def keyboard(key, x, y):

    global type_primitive
    global mode

    if key == b'\x1b'or key == b'q':
        glut.glutLeaveMainLoop()
    if key == b'1':
        mode = 1
    if key == b'2':
        mode = 2

    glut.glutPostRedisplay()

## Idle function.
#
# Called continuously.
def idle():
    global angle

    angle = angle+angle_inc if (angle+angle_inc) < 360.0 else (360.0-angle+angle_inc)

    glut.glutPostRedisplay()


## Init vertex data.
#
# Defines the coordinates for vertices, creates the arrays for OpenGL.
def initData():

    # Uses vertex arrays.
    global VAO
    global VBO

    # Set vertices.
    vertices = np.array([
        # First triangle
        # coordinate     color
        -0.5, -0.5, 0.0, 0.0, 0.0, 1.0,
         0.5, -0.5, 0.0, 1.0, 0.0, 0.0,
        -0.5,  0.5, 0.0, 1.0, 0.0, 0.0,
        # Second triangle
        # coordinate     color
         0.5, -0.5, 0.0, 1.0, 0.0, 0.0,
         0.5,  0.5, 0.0, 0.0, 1.0, 0.0,
        -0.5,  0.5, 0.0, 1.0, 0.0, 0.0
    ], dtype='float32')
    
    # Vertex array.
    VAO = gl.glGenVertexArrays(1)
    gl.glBindVertexArray(VAO)

    # Vertex buffer
    VBO = gl.glGenBuffers(1)
    gl.glBindBuffer(gl.GL_ARRAY_BUFFER, VBO)
    gl.glBufferData(gl.GL_ARRAY_BUFFER, vertices.nbytes, vertices, gl.GL_STATIC_DRAW)
    
    # Set attributes.
    gl.glVertexAttribPointer(0, 3, gl.GL_FLOAT, gl.GL_FALSE, 6*vertices.itemsize, None)
    gl.glEnableVertexAttribArray(0)
    gl.glVertexAttribPointer(1, 3, gl.GL_FLOAT, gl.GL_FALSE, 6*vertices.itemsize, c_void_p(3*vertices.itemsize))
    gl.glEnableVertexAttribArray(1)
    
    # Unbind Vertex Array Object.
    gl.glBindVertexArray(0)

## Create program (shaders).
#
# Compile shaders and create programs.
def initShaders():

    global program

    program = ut.createShaderProgram(vertex_code, fragment_code)


## Main function.
#
# Init GLUT and the window settings. Also, defines the callback functions used in the program.
def main():

    glut.glutInit()
    glut.glutInitContextVersion(3, 3);
    glut.glutInitContextProfile(glut.GLUT_CORE_PROFILE);
    glut.glutInitDisplayMode(glut.GLUT_DOUBLE | glut.GLUT_RGBA)
    glut.glutInitWindowSize(win_width,win_height)
    glut.glutCreateWindow('Geometric transformations')

    # Init vertex data for the triangle.
    initData()
    
    # Create shaders.
    initShaders()

    glut.glutReshapeFunc(reshape)
    glut.glutDisplayFunc(display)
    glut.glutKeyboardFunc(keyboard)
    glut.glutIdleFunc(idle);

    glut.glutMainLoop()

if __name__ == '__main__':
    main()
