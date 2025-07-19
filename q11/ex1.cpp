// circle_raster.cpp
// Rasteriza um círculo usando o algoritmo do ponto médio (Midpoint Circle Algorithm).
// Compile com:
// g++ circle_raster.cpp -o circle_raster -lGL -lGLU -lGLEW -lglut

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <vector>
#include <iostream>

// Janela
int winW = 800, winH = 600;

// Círculo: centro (pixel) e raio (pixel)
int xc = winW/2;
int yc = winH/2;
int radius = 100;

// Pontos do círculo em NDC
std::vector<glm::vec2> circlePoints;

// Shaders simples para desenhar pontos
const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
void main(){
    gl_PointSize = 2.0;
    gl_Position = vec4(aPos, 0, 1);
}
)";
const char* fragSrc = R"(
#version 330 core
out vec4 FragColor;
void main(){
    FragColor = vec4(1,1,0,1); // amarelo
}
)";

GLuint prog, vao, vbo;

// Compila e linka programa GLSL
GLuint compileProgram(){
    auto compile = [&](GLenum type,const char* src){
        GLuint s = glCreateShader(type);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p,vs);
    glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// Converte coordenada de pixel para NDC
glm::vec2 pxToNDC(int x, int y){
    float nx =  2.0f * x / float(winW) - 1.0f;
    float ny = -2.0f * y / float(winH) + 1.0f;
    return {nx, ny};
}

//algoritmo do ponto médio
/* Ele se baseia na equação do círculo x^2+y^2=r^2, mas evita cálculos 
complexos como raízes quadradas ou funções trigonométricas.

A ideia é começar do ponto mais alto do círculo (0,r)(0,r) e, a cada passo, 
decidir entre dois possíveis pixels vizinhos para aproximar melhor a curva 
do círculo. Essa decisão é feita por uma função de erro (ou decisão), que 
calcula qual dos dois pontos está mais próximo da borda real do círculo.

Se d < 0: o pixel abaixo da curva é melhor → só aumenta x

Se d >= 0: o pixel diagonal está mais perto → aumenta x e diminui y

Devido à simetria do círculo, o algoritmo calcula apenas um oitavo do círculo 
e reflete os pontos para os outros sete octantes, garantindo precisão e eficiência.
*/

void rasterizeCircle(){
    int x = 0;
    int y = radius;
    int d = 1 - radius;
    auto plot8 = [&](int xi,int yi){
        circlePoints.push_back(pxToNDC(xc + xi, yc + yi));
        circlePoints.push_back(pxToNDC(xc - xi, yc + yi));
        circlePoints.push_back(pxToNDC(xc + xi, yc - yi));
        circlePoints.push_back(pxToNDC(xc - xi, yc - yi));
        circlePoints.push_back(pxToNDC(xc + yi, yc + xi));
        circlePoints.push_back(pxToNDC(xc - yi, yc + xi));
        circlePoints.push_back(pxToNDC(xc + yi, yc - xi));
        circlePoints.push_back(pxToNDC(xc - yi, yc - xi));
    };
    while(x <= y){
        plot8(x,y);
        if(d < 0){
            d += 2*x + 3;
        } else {
            d += 2*(x - y) + 5;
            y--;
        }
        x++;
    }
}

// Inicialização do OpenGL
void initGL(){
    prog = compileProgram();
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);

    rasterizeCircle();

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 circlePoints.size()*sizeof(glm::vec2),
                 circlePoints.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glPointSize(2.0f);
}

// Callback display
void display(){
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(prog);
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, circlePoints.size());

    glutSwapBuffers();
}

// Redimensiona viewport
void reshape(int w,int h){
    winW = w; winH = h;
    glViewport(0,0,w,h);
    // atualizar círculo em caso de mudança de tamanho
    circlePoints.clear();
    xc = winW/2; yc = winH/2;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    rasterizeCircle();
    glBufferData(GL_ARRAY_BUFFER,
                 circlePoints.size()*sizeof(glm::vec2),
                 circlePoints.data(),
                 GL_STATIC_DRAW);
}

void keyboard(unsigned char key, int x, int y) {
    float step = 0.5f;
    switch (key) {
        case 27 : glutLeaveMainLoop(); //exit(0); break;
        case 'q':
        case 'Q': glutLeaveMainLoop();
    }
    glutPostRedisplay();
}

int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
    glutInitWindowSize(winW,winH);
    glutCreateWindow("Rasterização de Círculo - Midpoint Algorithm");

    glewInit();
    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
