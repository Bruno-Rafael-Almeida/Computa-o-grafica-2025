#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Shader simples
const char* vertexSrc = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aTex;
uniform mat4 MVP;
out vec2 TexCoord;
void main(){
    TexCoord = aTex;
    gl_Position = MVP * vec4(aPos,1);
}
)";
const char* fragmentSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D ourTexture;
void main(){
    FragColor = texture(ourTexture, TexCoord);
}
)";

GLuint shaderProgram, VAO, textureID;
float angle = 0;

// Cria e compila shader
GLuint compileProgram(){
    auto compile = [&](GLenum tp, const char* src){
        GLuint s = glCreateShader(tp);
        glShaderSource(s,1,&src,nullptr);
        glCompileShader(s);
        return s;
    };
    GLuint vs = compile(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragmentSrc);
    GLuint p = glCreateProgram();
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

// Configura cubo (posições + UVs)
void setupCube(){
    /*
    float data[] = {
        // positions        // texcoords
        -1,-1,-1,  0,0,  1,-1,-1, 1,0,  1, 1,-1, 1,1,  -1, 1,-1, 0,1,
        -1,-1, 1,  0,0,  1,-1, 1, 1,0,  1, 1, 1, 1,1,  -1, 1, 1, 0,1,
        // ... repita para as 6 faces ...
    };*/
    float data[] = {
        // -- Face Traseira (z = -1)
        -1, -1, -1,   0, 0,
         1, -1, -1,   1, 0,
         1,  1, -1,   1, 1,
        -1,  1, -1,   0, 1,

        // -- Face Frontal (z = +1)
        -1, -1,  1,   0, 0,
         1, -1,  1,   1, 0,
         1,  1,  1,   1, 1,
        -1,  1,  1,   0, 1,

        // -- Face Esquerda (x = -1)
        -1, -1, -1,   0, 0,
        -1,  1, -1,   1, 0,
        -1,  1,  1,   1, 1,
        -1, -1,  1,   0, 1,

        // -- Face Direita (x = +1)
         1, -1, -1,   0, 0,
         1,  1, -1,   1, 0,
         1,  1,  1,   1, 1,
         1, -1,  1,   0, 1,

        // -- Face Inferior (y = -1)
        -1, -1, -1,   0, 0,
         1, -1, -1,   1, 0,
         1, -1,  1,   1, 1,
        -1, -1,  1,   0, 1,

        // -- Face Superior (y = +1)
        -1,  1, -1,   0, 0,
         1,  1, -1,   1, 0,
         1,  1,  1,   1, 1,
        -1,  1,  1,   0, 1,
    };

    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    GLuint VBO; glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(data),data,GL_STATIC_DRAW);
    // posição
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    // UV
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
}

// Carrega textura com stb_image
void loadTexture(const char* path){
    int w,h,ch;
    unsigned char* img = stbi_load(path,&w,&h,&ch,0);
    if(!img){ std::cerr<<"Failed to load texture\n"; exit(1); }
    glGenTextures(1,&textureID);
    glBindTexture(GL_TEXTURE_2D,textureID);
    GLenum fmt = (ch==4?GL_RGBA:GL_RGB);
    glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,img);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(img);
}

void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);

    // Matriz MVP
    glm::mat4 P=glm::perspective(glm::radians(45.0f),800.f/600.f,0.1f,100.f);
    glm::mat4 V=glm::translate(glm::mat4(1),glm::vec3(0,0,-5));
    angle += 0.5f;
    glm::mat4 M=glm::rotate(glm::mat4(1),glm::radians(angle),glm::vec3(1,1,0));
    glm::mat4 MVP = P*V*M;
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram,"MVP"),1,GL_FALSE,glm::value_ptr(MVP));

    // Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,textureID);
    glUniform1i(glGetUniformLocation(shaderProgram,"ourTexture"),0);

    glBindVertexArray(VAO);
    glDrawArrays(GL_QUADS,0,24);  // 6 faces × 4 vértices
    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'q':
        case 'Q': glutLeaveMainLoop();
        case 27: glutLeaveMainLoop(); break;
    }
    glutPostRedisplay();
}


int main(int argc,char** argv){
    if(argc<2){ std::cerr<<"Uso: "<<argv[0]<<" textura.png\n"; return 1;}
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutCreateWindow("Cubo Texturizado");
    glewInit();

    shaderProgram = compileProgram();
    setupCube();
    loadTexture(argv[1]);
    glutKeyboardFunc(keyboard);
    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutMainLoop();
    return 0;
}
