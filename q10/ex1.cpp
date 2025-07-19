// clip.cpp
// Compile com:
// g++ clip.cpp -o clip -lGL -lGLU -lGLEW -lglut
/*

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include <iostream>
#include <functional>  // std::function
#include <algorithm>   // glm::min/glm::max

// Estágios de entrada
enum Stage { RECT, POLY, DONE };
Stage stage = RECT;

// Dimensões da janela
int winW = 800, winH = 600;

// Retângulo de recorte (em NDC)
glm::vec2 rectMin, rectMax;
bool rectDefined = false;

// Polígonos original e recortado
std::vector<glm::vec2> poly, clipped;

// Shaders GLSL simples
const char* vertSrc = R"(
#version 330 core
layout(location=0) in vec2 aPos;
uniform vec3 uColor;
out vec3 vColor;
void main(){
    vColor = uColor;
    gl_Position = vec4(aPos, 0, 1);
}
)";
const char* fragSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main(){
    fragColor = vec4(vColor, 1);
}
)";

GLuint prog, vao, vbo;

// Compila e linka um programa GLSL
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

// Converte coordenadas de tela em NDC [-1..1]
glm::vec2 screenToNDC(int x, int y){
    float nx =  2.0f * x / float(winW) - 1.0f;
    float ny = -2.0f * y / float(winH) + 1.0f;
    return {nx, ny};
}

// Sutherland–Hodgman para uma única aresta
using Vec2 = glm::vec2;
std::vector<Vec2> clipEdge(const std::vector<Vec2>& in,
                           std::function<bool(Vec2)> inside,
                           std::function<Vec2(Vec2,Vec2)> intersect)
{
    std::vector<Vec2> out;
    int n = in.size();
    for(int i = 0; i < n; ++i){
        Vec2 A = in[i];
        Vec2 B = in[(i+1)%n];
        bool inA = inside(A), inB = inside(B);
        if(inA && inB){
            // ambos dentro
            out.push_back(B);
        } else if(inA && !inB){
            // sai
            out.push_back(intersect(A,B));
        } else if(!inA && inB){
            // entra
            out.push_back(intersect(A,B));
            out.push_back(B);
        }
        // ambos fora → nada
    }
    return out;
}

// Executa recorte contra as 4 arestas do retângulo
void doClipping(){
    clipped = poly;
    float xmin = rectMin.x, xmax = rectMax.x;
    float ymin = rectMin.y, ymax = rectMax.y;
    // esquerda x >= xmin
    clipped = clipEdge(clipped,
        [xmin](Vec2 v){ return v.x >= xmin; },
        [xmin](Vec2 A,Vec2 B){
            float t = (xmin - A.x) / (B.x - A.x);
            return A + t*(B - A);
        });
    // direita x <= xmax
    clipped = clipEdge(clipped,
        [xmax](Vec2 v){ return v.x <= xmax; },
        [xmax](Vec2 A,Vec2 B){
            float t = (xmax - A.x) / (B.x - A.x);
            return A + t*(B - A);
        });
    // inferior y >= ymin
    clipped = clipEdge(clipped,
        [ymin](Vec2 v){ return v.y >= ymin; },
        [ymin](Vec2 A,Vec2 B){
            float t = (ymin - A.y) / (B.y - A.y);
            return A + t*(B - A);
        });
    // superior y <= ymax
    clipped = clipEdge(clipped,
        [ymax](Vec2 v){ return v.y <= ymax; },
        [ymax](Vec2 A,Vec2 B){
            float t = (ymax - A.y) / (B.y - A.y);
            return A + t*(B - A);
        });
}

// Desenha um polígono (linha ou loop) com cor
void drawPoly(const std::vector<glm::vec2>& pts, GLenum mode, glm::vec3 color){
    if(pts.empty()) return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pts.size()*sizeof(glm::vec2), pts.data(), GL_DYNAMIC_DRAW);
    glUseProgram(prog);
    glUniform3f(glGetUniformLocation(prog,"uColor"), color.r, color.g, color.b);
    glBindVertexArray(vao);
    glDrawArrays(mode, 0, pts.size());
}

void display(){
    glClear(GL_COLOR_BUFFER_BIT);

    // 1) desenha retângulo em branco
    if(rectDefined){
        std::vector<glm::vec2> R = {
            {rectMin.x, rectMin.y},
            {rectMax.x, rectMin.y},
            {rectMax.x, rectMax.y},
            {rectMin.x, rectMax.y}
        };
        drawPoly(R, GL_LINE_LOOP, {1,1,1});
    }

    // 2) desenha polígono original em verde
    if(!poly.empty()){
        drawPoly(poly, GL_LINE_LOOP, {0,1,0});
    }

    // 3) desenha polígono recortado em vermelho
    if(stage == DONE){
        drawPoly(clipped, GL_LINE_LOOP, {1,0,0});
    }

    glutSwapBuffers();
}

void mouse(int button,int state,int x,int y){
    glm::vec2 p = screenToNDC(x,y);

    if(button==GLUT_LEFT_BUTTON && state==GLUT_DOWN){
        if(stage == RECT){
            if(!rectDefined){
                rectMin = p;
                rectDefined = true;
            } else {
                rectMax = p;
                // ordena corretamente
                glm::vec2 realMin = glm::min(rectMin, rectMax);
                glm::vec2 realMax = glm::max(rectMin, rectMax);
                rectMin = realMin;
                rectMax = realMax;
                stage = POLY;
            }
            glutPostRedisplay();
        }
        else if(stage == POLY){
            poly.push_back(p);
            glutPostRedisplay();
        }
    }

    if(button==GLUT_RIGHT_BUTTON && state==GLUT_DOWN){
        if(stage==POLY && poly.size()>=3){
            stage = DONE;
            doClipping();
            glutPostRedisplay();
        }
    }
}

void initGL(){
    prog = compileProgram();
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
}

int main(int argc,char**argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Clipping Sutherland–Hodgman");
    glewInit();
    initGL();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMainLoop();
    return 0;
}
*/
/*
#include <iostream>
#include <vector>
#include <algorithm> // Para std::min e std::max
#include <GL/glut.h> // Para GLUT
#include <GL/glu.h>  // Para gluOrtho2D
#include <GL/freeglut_ext.h>

// --- Estrutura para Pontos ---
struct Point {
    float x, y;
};

// --- Configurações da Janela ---
int windowWidth = 800;
int windowHeight = 600;

// --- Estados do Programa ---
enum State {
    DEFINE_RECT_P1,   // Esperando o 1º clique para o retângulo
    DEFINE_RECT_P2,   // Esperando o 2º clique para o retângulo
    DEFINE_POLYGON,   // Esperando cliques para o polígono
    CLIPPED           // Polígono definido, recortado e pronto para exibição
};

State current_state = DEFINE_RECT_P1;
Point clipRectPoints[2];         // Guarda os dois pontos brutos do clique
float clipRectNormalized[4];     // (xmin, ymin, xmax, ymax) do retângulo de recorte
int rectClickCount = 0;          // Contador de cliques para o retângulo

std::vector<Point> polygonVertices;         // Vértices do polígono original
std::vector<Point> clippedPolygonVertices;  // Vértices do polígono recortado

// --- Funções Auxiliares Sutherland-Hodgman ---

// Retorna true se o ponto está "dentro" da aresta de clipping
bool isInside(Point p, float clipValue, char edgeType) {
    if (edgeType == 'L') return p.x >= clipValue;  // Esquerda: x >= xmin
    if (edgeType == 'R') return p.x <= clipValue;  // Direita: x <= xmax
    if (edgeType == 'B') return p.y >= clipValue;  // Baixo: y >= ymin
    if (edgeType == 'T') return p.y <= clipValue;  // Cima: y <= ymax
    return false;
}

// Calcula o ponto de interseção de um segmento (p1, p2) com uma aresta de clipping
Point intersect(Point p1, Point p2, float clipValue, char edgeType) {
    Point intersection;
    float slope;

    if (edgeType == 'L' || edgeType == 'R') { // Aresta vertical (x = clipValue)
        if (p2.x - p1.x == 0) { // Segmento vertical, retorna p1 (assumindo que está na linha)
            return p1;
        }
        slope = (p2.y - p1.y) / (p2.x - p1.x);
        intersection.x = clipValue;
        intersection.y = p1.y + slope * (clipValue - p1.x);
    } else { // Aresta horizontal (y = clipValue)
        if (p2.y - p1.y == 0) { // Segmento horizontal, retorna p1
            return p1;
        }
        slope = (p2.x - p1.x) / (p2.y - p1.y);
        intersection.y = clipValue;
        intersection.x = p1.x + slope * (clipValue - p1.y);
    }
    return intersection;
}

// Algoritmo de Sutherland-Hodgman
std::vector<Point> clipPolygonSutherlandHodgman(const std::vector<Point>& poly, float clipRectNorm[4]) {
    if (poly.empty()) {
        return {};
    }

    float xmin = clipRectNorm[0];
    float ymin = clipRectNorm[1];
    float xmax = clipRectNorm[2];
    float ymax = clipRectNorm[3];

    // Define as arestas de clipping (tipo, valor)
    std::vector<std::pair<char, float>> clipEdges = {
        {'L', xmin}, // Left
        {'R', xmax}, // Right
        {'B', ymin}, // Bottom
        {'T', ymax}  // Top
    };

    std::vector<Point> clippedPoly = poly; // Começa com o poligono de entrada

    for (const auto& edge : clipEdges) {
        char edgeType = edge.first;
        float clipValue = edge.second;

        if (clippedPoly.empty()) { // Se o poligono se tornou vazio em uma iteracao anterior
            return {};
        }

        std::vector<Point> inputPolyForEdge = clippedPoly;
        clippedPoly.clear(); // Reinicia a lista para a saida desta aresta

        Point s = inputPolyForEdge.back(); // Pega o ultimo ponto para o "wrap-around"

        for (const auto& p : inputPolyForEdge) {
            bool s_in = isInside(s, clipValue, edgeType);
            bool p_in = isInside(p, clipValue, edgeType);

            if (p_in) { // Caso 1: P está dentro da aresta de clipping
                if (!s_in) { // S está fora, P está dentro (S -> Intersect -> P)
                    clippedPoly.push_back(intersect(s, p, edgeType, clipValue));
                }
                clippedPoly.push_back(p); // S dentro, P dentro (S -> P) ou Intersect -> P
            } else { // Caso 2: P está fora da aresta de clipping
                if (s_in) { // S está dentro, P está fora (S -> Intersect)
                    clippedPoly.push_back(intersect(s, p, edgeType, clipValue));
                }
                // S fora, P fora (não adiciona nada)
            }
            s = p; // Atualiza s para o próximo segmento
        }
    }
    return clippedPoly;
}

// --- Funções de Desenho OpenGL ---

void drawRectangle(float xmin, float ymin, float xmax, float ymax) {
    glColor3f(0.0f, 0.0f, 1.0f); // Azul
    glBegin(GL_LINE_LOOP);
    glVertex2f(xmin, ymin);
    glVertex2f(xmax, ymin);
    glVertex2f(xmax, ymax);
    glVertex2f(xmin, ymax);
    glEnd();
}

void drawPolygon(const std::vector<Point>& vertices, float r, float g, float b, GLenum mode) {
    glColor3f(r, g, b);
    glBegin(mode);
    for (const auto& p : vertices) {
        glVertex2f(p.x, p.y);
    }
    glEnd();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Desenhar o retângulo de clipping
    if (rectClickCount == 2) {
        drawRectangle(clipRectNormalized[0], clipRectNormalized[1], clipRectNormalized[2], clipRectNormalized[3]);
    } else if (rectClickCount == 1) { // Desenha o primeiro ponto enquanto espera o segundo
        glColor3f(0.0f, 0.0f, 1.0f);
        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glVertex2f(clipRectPoints[0].x, clipRectPoints[0].y);
        glEnd();
        glPointSize(1.0f);
    }

    // Desenhar o polígono original (se em definição ou finalizado)
    if (current_state >= DEFINE_POLYGON && !polygonVertices.empty()) {
        drawPolygon(polygonVertices, 0.0f, 1.0f, 0.0f, GL_LINE_LOOP); // Verde
    }

    // Desenhar o polígono recortado (se finalizado)
    if (current_state == CLIPPED && !clippedPolygonVertices.empty()) {
        drawPolygon(clippedPolygonVertices, 1.0f, 0.0f, 0.0f, GL_LINE_LOOP); // Vermelho
        // Opcional: para visualizar o poligono recortado preenchido se ele for valido
        // if (clippedPolygonVertices.size() >= 3) { // Precisa de pelo menos 3 vertices para ser um poligono
        //     drawPolygon(clippedPolygonVertices, 1.0f, 0.0f, 0.0f, GL_POLYGON);
        // }
    }

    glFlush(); // Garante que os comandos OpenGL são executados
}

// --- Callbacks de Eventos ---

void mouse(int button, int state, int x, int y) {
    // Converter coordenadas do mouse (origem superior esquerda) para OpenGL (origem inferior esquerda)
    float opengl_y = (float)windowHeight - (float)y;
    Point clickPoint = {(float)x, opengl_y};

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (current_state == DEFINE_RECT_P1) {
            clipRectPoints[0] = clickPoint;
            rectClickCount = 1;
            current_state = DEFINE_RECT_P2;
            std::cout << "Retangulo: Primeiro ponto em (" << x << ", " << y << ")" << std::endl;
        } else if (current_state == DEFINE_RECT_P2) {
            clipRectPoints[1] = clickPoint;
            rectClickCount = 2;
            // Normaliza os pontos para (xmin, ymin, xmax, ymax)
            clipRectNormalized[0] = std::min(clipRectPoints[0].x, clipRectPoints[1].x); // xmin
            clipRectNormalized[1] = std::min(clipRectPoints[0].y, clipRectPoints[1].y); // ymin
            clipRectNormalized[2] = std::max(clipRectPoints[0].x, clipRectPoints[1].x); // xmax
            clipRectNormalized[3] = std::max(clipRectPoints[0].y, clipRectPoints[1].y); // ymax
            current_state = DEFINE_POLYGON;
            std::cout << "Retangulo: Segundo ponto em (" << x << ", " << y << "). Retangulo definido: ("
                      << clipRectNormalized[0] << ", " << clipRectNormalized[1] << ", "
                      << clipRectNormalized[2] << ", " << clipRectNormalized[3] << ")" << std::endl;
            std::cout << "Clique esquerdo para definir vertices do poligono. Botao direito para finalizar." << std::endl;
        } else if (current_state == DEFINE_POLYGON) {
            polygonVertices.push_back(clickPoint);
            std::cout << "Poligono: Vertice adicionado em (" << x << ", " << y << "). Total: "
                      << polygonVertices.size() << std::endl;
        } else { // current_state == CLIPPED, permite redefinir o retangulo ou poligono
            std::cout << "Programa finalizado. Limpando e recomecando." << std::endl;
            current_state = DEFINE_RECT_P1;
            rectClickCount = 0;
            polygonVertices.clear();
            clippedPolygonVertices.clear();
        }
    } else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        if (current_state == DEFINE_POLYGON) {
            if (polygonVertices.size() < 3) {
                std::cout << "ERRO: O poligono precisa de no minimo 3 vertices!" << std::endl;
            } else {
                // Executa o algoritmo de recorte
                clippedPolygonVertices = clipPolygonSutherlandHodgman(polygonVertices, clipRectNormalized);
                current_state = CLIPPED;
                std::cout << "Poligono finalizado e recortado." << std::endl;
                std::cout << "Vertices do poligono original: ";
                for (const auto& p : polygonVertices) {
                    std::cout << "(" << p.x << ", " << p.y << ") ";
                }
                std::cout << std::endl;
                std::cout << "Vertices do poligono recortado: ";
                if (clippedPolygonVertices.empty()) {
                    std::cout << "Nenhum (poligono completamente recortado)";
                } else {
                    for (const auto& p : clippedPolygonVertices) {
                        std::cout << "(" << p.x << ", " << p.y << ") ";
                    }
                }
                std::cout << std::endl;
                if (!clippedPolygonVertices.empty() && clippedPolygonVertices.size() < 3) {
                    std::cout << "Aviso: O poligono recortado tem menos de 3 vertices e sera desenhado como linha(s)." << std::endl;
                }
                std::cout << "\nClique esquerdo novamente para recomecar." << std::endl;
            }
        }
    }
    glutPostRedisplay(); // Solicita uma nova renderizacao da cena
}

void reshape(int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height); // Define o sistema de coordenadas 2D
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 27 : glutLeaveMainLoop(); //exit(0); break;
        case 'q':
        case 'Q': glutLeaveMainLoop();
    }
    glutPostRedisplay();
}

// --- Inicialização OpenGL e GLUT ---
void initGL() {
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f); // Cor de fundo (cinza claro)
    glEnable(GL_BLEND); // Habilita mistura para transparencia (se usado)
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// --- Função Principal ---
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Algoritmo de Recorte Sutherland-Hodgman - C++");

    initGL();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    std::cout << "--- Recorte de Poligonos Sutherland-Hodgman ---" << std::endl;
    std::cout << "1. Clique esquerdo para definir o primeiro ponto do retangulo de recorte." << std::endl;
    std::cout << "2. Clique esquerdo para definir o segundo ponto do retangulo de recorte." << std::endl;
    std::cout << "3. Clique esquerdo para adicionar vertices ao poligono (minimo de 3)." << std::endl;
    std::cout << "4. Clique direito para finalizar o poligono e realizar o recorte." << std::endl;
    std::cout << "5. Pressione ESC para sair." << std::endl;

    glutMainLoop();

    return 0;
}*/

// g++ suth_clip.cpp -o suth_clip -lGL -lGLU -lglut -lGLEW

#include <GL/glew.h>
#include <GL/freeglut.h>
#include <vector>
#include <iostream>
#include <algorithm>

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}
};

std::vector<Vec2> polyPoints, clippedPoly;
Vec2 rectMin, rectMax;
bool drawingRect = true, polygonDone = false;
int clickCount = 0;

// Converte coordenadas da tela para NDC
Vec2 screenToNDC(int x, int y) {
    return Vec2((2.0f * x / 800.0f - 1.0f), (1.0f - 2.0f * y / 600.0f));
}

// Sutherland-Hodgman Clipping
std::vector<Vec2> clipEdge(std::vector<Vec2> input, Vec2 p1, Vec2 p2) {
    std::vector<Vec2> output;
    for (int i = 0; i < input.size(); ++i) {
        Vec2 cur = input[i];
        Vec2 prev = input[(i + input.size() - 1) % input.size()];
        float cp1 = (p2.x - p1.x) * (cur.y - p1.y) - (p2.y - p1.y) * (cur.x - p1.x);
        float cp2 = (p2.x - p1.x) * (prev.y - p1.y) - (p2.y - p1.y) * (prev.x - p1.x);

        if (cp1 >= 0 && cp2 >= 0) {
            output.push_back(cur);
        } else if (cp1 >= 0 && cp2 < 0) {
            float t = cp2 / (cp2 - cp1);
            output.push_back(Vec2(prev.x + t * (cur.x - prev.x), prev.y + t * (cur.y - prev.y)));
            output.push_back(cur);
        } else if (cp1 < 0 && cp2 >= 0) {
            float t = cp2 / (cp2 - cp1);
            output.push_back(Vec2(prev.x + t * (cur.x - prev.x), prev.y + t * (cur.y - prev.y)));
        }
    }
    return output;
}

void clipPolygon() {
    std::vector<Vec2> out = polyPoints;
    out = clipEdge(out, Vec2(rectMin.x, rectMin.y), Vec2(rectMin.x, rectMax.y)); // Left
    out = clipEdge(out, Vec2(rectMin.x, rectMax.y), Vec2(rectMax.x, rectMax.y)); // Top
    out = clipEdge(out, Vec2(rectMax.x, rectMax.y), Vec2(rectMax.x, rectMin.y)); // Right
    out = clipEdge(out, Vec2(rectMax.x, rectMin.y), Vec2(rectMin.x, rectMin.y)); // Bottom
    clippedPoly = out;
    glutPostRedisplay();
}

void display() {
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 0, 0); // Vermelho para retângulo

    if (clickCount >= 2) {
        glBegin(GL_LINE_LOOP);
        glVertex2f(rectMin.x, rectMin.y);
        glVertex2f(rectMin.x, rectMax.y);
        glVertex2f(rectMax.x, rectMax.y);
        glVertex2f(rectMax.x, rectMin.y);
        glEnd();
    }

    if (!polyPoints.empty()) {
        glColor3f(0, 0, 1); // Azul para polígono
        glBegin(GL_LINE_LOOP);
        for (auto& p : polyPoints)
            glVertex2f(p.x, p.y);
        glEnd();
    }

    if (!clippedPoly.empty()) {
        glColor3f(0, 1, 0); // Verde para polígono recortado
        glBegin(GL_POLYGON);
        for (auto& p : clippedPoly)
            glVertex2f(p.x, p.y);
        glEnd();
    }

    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    if (state != GLUT_DOWN) return;

    if (button == GLUT_LEFT_BUTTON) {
        Vec2 ndc = screenToNDC(x, y);
        if (drawingRect) {
            if (clickCount == 0) rectMin = ndc;
            else if (clickCount == 1) {
                rectMax = ndc;
                drawingRect = false;
            }
            clickCount++;
        } else if (!polygonDone) {
            polyPoints.push_back(ndc);
        }
    } else if (button == GLUT_RIGHT_BUTTON && !drawingRect && !polygonDone) {
        polygonDone = true;
        clipPolygon();
    }
    glutPostRedisplay();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
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

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Sutherland-Hodgman Clipping");
    glewInit();
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);
    glutMainLoop();
    return 0;
}



