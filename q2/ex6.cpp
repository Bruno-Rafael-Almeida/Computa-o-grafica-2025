
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <cmath>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Dimensões da janela
int win_width = 800;
int win_height = 600;

GLuint program;
GLuint VAO, VBO;

// Shaders
const char *vertex_code = R"(
#version 330 core
layout (location = 0) in vec3 position;
void main() {
    gl_Position = vec4(position, 1.0);
}
)";

const char *fragment_code = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

// Armazena pontos normalizados para OpenGL (-1 a 1)
std::vector<glm::vec2> points;

// Converte coordenadas de tela para NDC
glm::vec2 screenToNDC(int x, int y) {
    float ndc_x = 2.0f * x / win_width - 1.0f;
    float ndc_y = 1.0f - 2.0f * y / win_height;
    return glm::vec2(ndc_x, ndc_y);
}

// Desenha vetores
void display() {
    glClearColor(0.2, 0.3, 0.3, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    if (points.size() == 3) {
        std::vector<float> vertices = {
            points[1].x, points[1].y, 0.0f, // O
            points[0].x, points[0].y, 0.0f, // P
            points[1].x, points[1].y, 0.0f, // O
            points[2].x, points[2].y, 0.0f  // Q
        };

        
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
        glUseProgram(program);
        glBindVertexArray(VAO);

        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);
        
    }

    glutSwapBuffers();
}

// Cálculos geométricos
void computeVectors() {
    glutPostRedisplay();
    glm::vec2 P = points[0];
    glm::vec2 O = points[1];
    glm::vec2 Q = points[2];

    glm::vec2 u = P - O;
    glm::vec2 v = Q - O;

    float dot = glm::dot(u, v);
    float cross = u.x * v.y - u.y * v.x;
    float angle = std::atan2(cross, dot) * 180.0f / M_PI;
    float distance = std::abs(cross) / glm::length(v);

    std::cout << "Ponto P: (" << P.x << ", " << P.y << ")" << std::endl;
    std::cout << "Ponto O: (" << O.x << ", " << O.y << ")" << std::endl;
    std::cout << "Ponto Q: (" << Q.x << ", " << Q.y << ")" << std::endl;

    std::cout << "u = P - O = (" << u.x << ", " << u.y << ")\n";
    std::cout << "v = Q - O = (" << v.x << ", " << v.y << ")\n";
    std::cout << "Ângulo entre u e v = " << angle << " graus\n";
    std::cout << "Produto interno (u . v) = " << dot << "\n";
    std::cout << "Produto vetorial (u x v) em escalar = " << cross << "\n";
    std::cout << "Distância de P à reta OQ = " << distance << "\n";
}

// Clique do mouse
void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (points.size() < 3) {
            std::cout << "ponto " << x << " " << y << "\n";
            points.push_back(screenToNDC(x, y));
        } else {
            points.clear();
            points.push_back(screenToNDC(x, y));
        }

        if (points.size() == 3) {
            computeVectors();
        }

        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y)
{
        switch (key)
        {
                case 27:
                        glutLeaveMainLoop();
                case 'q':
                case 'Q':
                        glutLeaveMainLoop();
        }
}

void reshape(int w, int h) {
    win_width = w;
    win_height = h;
    glViewport(0, 0, w, h);
}


void initShaders()
{	
	program  = glCreateProgram();
    int vertex   = glCreateShader(GL_VERTEX_SHADER);
    int fragment = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vertex, 1, &vertex_code, NULL);
    glShaderSource(fragment, 1, &fragment_code, NULL);

    glCompileShader(vertex);
    glCompileShader(fragment);

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);

    glLinkProgram(program);

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    glUseProgram(program);
}

void initBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // Configura o layout do buffer para o atributo de posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(win_width, win_height);
    glutCreateWindow("Vetores com Mouse");

    glewInit();

    initShaders();

    initBuffers();

    glutDisplayFunc(display);
    glutMouseFunc(mouseClick);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);

    glutMainLoop();
    return 0;
}
