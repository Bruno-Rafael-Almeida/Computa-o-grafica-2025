#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

int win_width = 800, win_height = 600;
GLuint program, VAO, VBO;
float angle = 0.0f;
float angle_inc = 0.5f;
int mode = 3; // modo default: câmera orbitando

const char* vertex_code = R"(
#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
out vec3 vColor;
uniform mat4 transform;
void main() {
    gl_Position = transform * vec4(position, 1.0);
    vColor = color;
}
)";

const char* fragment_code = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

void initShaders() {
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertex, 1, &vertex_code, NULL);
    glShaderSource(fragment, 1, &fragment_code, NULL);
    glCompileShader(vertex);
    glCompileShader(fragment);

    program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    glUseProgram(program);
}

void initData() {
    float vertices[] = {
        // Posições           // Cores
        -0.5f, -0.5f, -0.5f,    1, 0, 0,
         0.5f, -0.5f, -0.5f,    1, 0, 0,
         0.5f,  0.5f, -0.5f,    1, 0, 0,
         0.5f,  0.5f, -0.5f,    1, 0, 0,
        -0.5f,  0.5f, -0.5f,    1, 0, 0,
        -0.5f, -0.5f, -0.5f,    1, 0, 0,

        -0.5f, -0.5f,  0.5f,    0, 1, 0,
         0.5f, -0.5f,  0.5f,    0, 1, 0,
         0.5f,  0.5f,  0.5f,    0, 1, 0,
         0.5f,  0.5f,  0.5f,    0, 1, 0,
        -0.5f,  0.5f,  0.5f,    0, 1, 0,
        -0.5f, -0.5f,  0.5f,    0, 1, 0,

        -0.5f,  0.5f,  0.5f,    0, 0, 1,
        -0.5f,  0.5f, -0.5f,    0, 0, 1,
        -0.5f, -0.5f, -0.5f,    0, 0, 1,
        -0.5f, -0.5f, -0.5f,    0, 0, 1,
        -0.5f, -0.5f,  0.5f,    0, 0, 1,
        -0.5f,  0.5f,  0.5f,    0, 0, 1,

         0.5f,  0.5f,  0.5f,    0.5, 0, 0.5,
         0.5f,  0.5f, -0.5f,    0.5, 0, 0.5,
         0.5f, -0.5f, -0.5f,    0.5, 0, 0.5,
         0.5f, -0.5f, -0.5f,    0.5, 0, 0.5,
         0.5f, -0.5f,  0.5f,    0.5, 0, 0.5,
         0.5f,  0.5f,  0.5f,    0.5, 0, 0.5,

        -0.5f, -0.5f, -0.5f,    0, 1, 0.5,
         0.5f, -0.5f, -0.5f,    0, 1, 0.5,
         0.5f, -0.5f,  0.5f,    0, 1, 0.5,
         0.5f, -0.5f,  0.5f,    0, 1, 0.5,
        -0.5f, -0.5f,  0.5f,    0, 1, 0.5,
        -0.5f, -0.5f, -0.5f,    0, 1, 0.5,

        -0.5f,  0.5f, -0.5f,    1, 0, 1,
         0.5f,  0.5f, -0.5f,    1, 0, 1,
         0.5f,  0.5f,  0.5f,    1, 0, 1,
         0.5f,  0.5f,  0.5f,    1, 0, 1,
        -0.5f,  0.5f,  0.5f,    1, 0, 1,
        -0.5f,  0.5f, -0.5f,    1, 0, 1
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void reshape(int w, int h) {
    win_width = w;
    win_height = h;
    glViewport(0, 0, w, h);
}

void display() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5, 0.5, 0.5, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program);
    glBindVertexArray(VAO);

    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = glm::mat4(1.0f);

    if (mode == 1) {
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.8f, 1.0f, 0.0f));
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    } else if (mode == 2) {
        model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        view = glm::translate(view, glm::vec3(0.0f, 0.0f, -3.0f));
    } else if (mode == 3) {
        float radius = 3.0f;
        float camX = radius * sin(glm::radians(angle));
        float camZ = radius * cos(glm::radians(angle));
        view = glm::lookAt(glm::vec3(camX, 1.5f, camZ), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)win_width / win_height, 0.1f, 100.0f);
    glm::mat4 transform = projection * view * model;

    GLuint loc = glGetUniformLocation(program, "transform");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));

    glDrawArrays(GL_TRIANGLES, 0, 36);
    glutSwapBuffers();
}

void idle() {
    angle += angle_inc;
    if (angle > 360.0f) angle -= 360.0f;
    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == '1') mode = 1;
    if (key == '2') mode = 2;
    if (key == '3') mode = 3;
    if (key == 'q' || key == 'Q') glutLeaveMainLoop();
    if (key == 27) exit(0);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(win_width, win_height);
    glutCreateWindow("Cubo com Camera Orbitando");

    glewInit();
    initData();
    initShaders();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);

    glutMainLoop();
    return 0;
}
