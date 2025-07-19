#include <iostream>
#include <cmath>
#include <vector>

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

using namespace std;

// Armazena os pontos clicados
vector<glm::vec2> points;

// Função para converter coordenadas da janela para sistema OpenGL
glm::vec2 screenToWorld(int x, int y, int width, int height) {
    float nx = (2.0f * x) / width - 1.0f;
    float ny = 1.0f - (2.0f * y) / height;
    return glm::vec2(nx, ny);
}

// Calcula e imprime os resultados
void computeVectors() {
    glm::vec2 P = points[0];
    glm::vec2 O = points[1];
    glm::vec2 Q = points[2];

    glm::vec2 u = P - O;
    glm::vec2 v = Q - O;

    float dot = glm::dot(u, v);
    float angle_rad = glm::angle(u, v);
    float angle_deg = glm::degrees(angle_rad);

    float cross = u.x * v.y - u.y * v.x;

    // Distância de P à linha (OQ)
    glm::vec2 v_norm = glm::normalize(v);
    glm::vec2 proj = glm::dot(u, v_norm) * v_norm;
    glm::vec2 perp = u - proj;
    float dist = glm::length(perp);

    cout << "\n--- Cálculos ---" << endl;
    cout << "u = P - O = (" << u.x << ", " << u.y << ")" << endl;
    cout << "v = Q - O = (" << v.x << ", " << v.y << ")" << endl;
    cout << "Produto Interno (u.v): " << dot << endl;
    cout << "Produto Vetorial (u x v): " << cross << endl;
    cout << "Ângulo entre u e v: " << angle_deg << " graus" << endl;
    cout << "Distância de P à linha definida por v: " << dist << endl;
    cout << "----------------\n" << endl;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glPointSize(6.0f);

    glBegin(GL_POINTS);
    for (const auto& p : points)
        glVertex2f(p.x, p.y);
    glEnd();

    // Desenha os vetores u = P - O e v = Q - O
    if (points.size() == 3) {
        glm::vec2 P = points[0];
        glm::vec2 O = points[1];
        glm::vec2 Q = points[2];

        glBegin(GL_LINES);
        // Vetor u (P - O)
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(O.x, O.y);
        glVertex2f(P.x, P.y);

        // Vetor v (Q - O)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex2f(O.x, O.y);
        glVertex2f(Q.x, Q.y);
        glEnd();
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN && points.size() < 3) {
        glm::vec2 p = screenToWorld(x, y, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
        points.push_back(p);
        glutPostRedisplay();

        if (points.size() == 3) {
            computeVectors();
        }
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Geometria com OpenGL");

    glewInit();

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutReshapeFunc(reshape);

    glutMainLoop();
    return 0;
}
