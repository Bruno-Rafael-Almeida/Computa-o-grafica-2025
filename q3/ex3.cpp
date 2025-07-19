#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

GLuint shaderProgram;
GLuint VAO, VBO;

float scale = 1.0f;
bool increasing = true;

const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in vec3 color;
out vec3 vColor;
uniform mat4 transform;
void main() {
    gl_Position = transform * vec4(position, 0.0, 1.0);
    vColor = color;
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

void compileShaders() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    
    glShaderSource(vertexShader, 1, &vertexShaderSrc, nullptr);
    glShaderSource(fragmentShader, 1, &fragmentShaderSrc, nullptr);
    
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void initData() {
    float vertices[] = {
        // posicao     // cor
        -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // bottom-left
         0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // bottom-right
         0.5f,  0.5f,  1.0f, 0.0f, 0.0f, // top-right

         0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // top-right
        -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // top-left
        -0.5f, -0.5f,  0.0f, 1.0f, 0.0f  // bottom-left
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // pos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void display() {
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    // escala animada
    glm::mat4 transform = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, 1.0f));
    GLuint loc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));

    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glutSwapBuffers();
}

void idle() {
    // varia a escala de forma suave entre 0.5 e 1.5
    if (increasing) {
        scale += 0.01f;
        if (scale >= 1.5f) increasing = false;
    } else {
        scale -= 0.01f;
        if (scale <= 0.5f) increasing = true;
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) exit(0); // ESC para sair
    if (key == 'q' || key == 'Q') glutLeaveMainLoop();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Retângulo com Escala Iterativa");

    glewInit();

    compileShaders();
    initData();

    glutDisplayFunc(display);
    glutIdleFunc(idle);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
