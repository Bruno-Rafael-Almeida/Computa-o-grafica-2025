#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/gtx/string_cast.hpp>

GLuint program, VAO, VBO;
std::vector<float> vertices;
int drawMode = GL_FILL;

glm::vec3 center(0.0f), translation(0.0f);
float scaleFactor = 1.0f;
glm::quat rotationQuat = glm::quat(1, 0, 0, 0);
bool leftMouseDown = false;
glm::vec2 lastMousePos;

const char *vertexShaderSource = "\n"
"#version 330 core\n"
"layout(location = 0) in vec3 aPos;\n"
"layout(location = 1) in vec3 aColor;\n"
"\n"
"out vec3 vertexColor;\n"
"\n"
"uniform mat4 transform;\n"
"\n"
"void main() {\n"
    "vertexColor = aColor;\n"
    "gl_Position = transform * vec4(aPos, 1.0);\n"
"}\n";

const char *fragmentShaderSource = "\n"
"#version 330 core\n"
"in vec3 vertexColor;\n"
"out vec4 FragColor;\n"
"\n"
"void main() {\n"
    "float gray = dot(vertexColor, vec3(0.299, 0.587, 0.114));\n"
    "FragColor = vec4(vec3(gray), 1.0);\n"
    "//FragColor = vec4(vertexColor, 1.0);\n"
"}\n";

//funcao para converter coordenadas da tela(pixels) para -1,1
glm::vec2 getTrackballVector(int x, int y, int width, int height) {
    float nx = (2.0f * x - width) / width;
    float ny = (height - 2.0f * y) / height;
    return glm::vec2(nx, ny);
}

//funcao para abrir o arquivo obj, calula as normais p/ usar como rgb
void loadModel(const std::string& path) {
    Assimp::Importer importer;

    //carregador de arquivo
    //aiProcess_Triangulate converte as facrs para tringulos
    //aiProcess_JoinIdenticalVertices remove vertices duplicados
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Erro ao carregar modelo: " << path << std::endl;
        exit(1);
    }

    const aiMesh* mesh = scene->mMeshes[0];
    vertices.clear();

    aiVector3D minV(1e10f), maxV(-1e10f);

    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) continue; // só triângulos

        aiVector3D v[3];
        for (int j = 0; j < 3; ++j)
            v[j] = mesh->mVertices[face.mIndices[j]];

        // Normal da face (direção da face)
        aiVector3D edge1 = v[1] - v[0];
        aiVector3D edge2 = v[2] - v[0];
        aiVector3D normal = edge1 ^ edge2; // produto vetorial
        normal.Normalize(); //transforma a normal em um vetor unitario

        // Mapeamento da normal para cor [0,1]
        float r = (normal.x + 1.0f) / 2.0f;
        float g = (normal.y + 1.0f) / 2.0f;
        float b = (normal.z + 1.0f) / 2.0f;

        // Para cada vértice da face: posição + cor
        for (int j = 0; j < 3; ++j) {
            vertices.push_back(v[j].x);
            vertices.push_back(v[j].y);
            vertices.push_back(v[j].z);

            vertices.push_back(r);
            vertices.push_back(g);
            vertices.push_back(b);

            minV.x = std::min(minV.x, v[j].x);
            minV.y = std::min(minV.y, v[j].y);
            minV.z = std::min(minV.z, v[j].z);

            maxV.x = std::max(maxV.x, v[j].x);
            maxV.y = std::max(maxV.y, v[j].y);
            maxV.z = std::max(maxV.z, v[j].z);
        }
    }

    // usado para centralizar o objeto na tela do opengl
    center = glm::vec3(
        (minV.x + maxV.x) / 2.0f,
        (minV.y + maxV.y) / 2.0f,
        (minV.z + maxV.z) / 2.0f
    );

    //para calcular o fator de escala do objeto
    float maxExtent = std::max({ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z });
    scaleFactor = 2.0f / maxExtent;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

//carregar e compilars os shaders
void compileShaders() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);
}

void display() {
    glEnable(GL_DEPTH_TEST); //para profundidade
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, drawMode);
    glUseProgram(program);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
    model = model * glm::toMat4(rotationQuat);
    model = glm::scale(model, glm::vec3(scaleFactor));
    model = glm::translate(model, -center);
    glm::mat4 transform = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(VAO);
    //pular 6 pois inclui as cores no carregamento do objeto
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);

    glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
    float step = 0.5f;
    switch (key) {
        case 'v': drawMode = (drawMode == GL_FILL ? GL_LINE : GL_FILL); break;
        case '+': scaleFactor *= 1.1f; break;
        case '-': scaleFactor *= 0.9f; break;
        case 27 : glutLeaveMainLoop(); //exit(0); break;
        case 'q':
        case 'Q': glutLeaveMainLoop();
        case 'w':
        case 'W': translation.z += step; break;
        case 'S':
        case 's': translation.z -= step; break;
    }
    glutPostRedisplay();
}

void specialKeys(int key, int, int) {
    float step = 0.5f;
    switch (key) {
        case GLUT_KEY_UP: translation.y += step; break;
        case GLUT_KEY_DOWN: translation.y -= step; break;
        case GLUT_KEY_LEFT: translation.x -= step; break;
        case GLUT_KEY_RIGHT: translation.x += step; break;
    }
    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) //verifica se o botao esquerdo foi precionado
        leftMouseDown = (state == GLUT_DOWN); //se o estado do botao e clicado

    if (button == 3) scaleFactor *= 1.1f; // Scroll cima
    if (button == 4) scaleFactor *= 0.9f; // Scroll baixo

    glutPostRedisplay();
    lastMousePos = glm::vec2(x, y); //ultima posicao do mouse
}

void motion(int x, int y) {
    if (!leftMouseDown) return; //verifica se o botao esquedo esta clicado

    glm::vec2 cur = getTrackballVector(x, y, 800, 600);
    glm::vec2 prev = getTrackballVector(lastMousePos.x, lastMousePos.y, 800, 600);

    //interpreta os pontos 2D para o 3D sobre uma esfera unitaria
    glm::vec3 a = glm::vec3(prev.x, prev.y, sqrtf(std::max(0.0f, 1 - prev.x * prev.x - prev.y * prev.y)));
    glm::vec3 b = glm::vec3(cur.x, cur.y, sqrtf(std::max(0.0f, 1 - cur.x * cur.x - cur.y * cur.y)));
    glm::vec3 axis = glm::cross(a, b); //eixo de rotacao, o produto vetorial de b para a
    float angle = acosf(std::min(1.0f, glm::dot(a, b))); //calcula o angulo de rotacao com produto escalar, e min p evitar erros de precisao quando 1 >

    rotationQuat = glm::normalize(glm::angleAxis(angle, axis) * rotationQuat); //gera o quaternion com o angulo
    lastMousePos = glm::vec2(x, y);
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    const char *h = "-h";
    if(strcmp(argv[1], h) == 0){
        std::cout << "CONTROLES DE MANIPULAÇÃO DA MALHA\n";
        std::cout << "Abertura da malha: ./mesh caminho/do/arquivo.obj\n";
        std::cout << "Letra v: alterna entre visualização de faces e wireframe\n";
        std::cout << "letra w: deslocamento positivo de Z\n";
        std::cout << "Letra s: deslocamento negativo de Y\n";
        std::cout << "Seta para cima: deslocamento positivo de Y\n";
        std::cout << "Seta para baixo: deslocamento negativo de Y\n";
        std::cout << "Seta para direita: deslocamento positivo de X\n";
        std::cout << "Seta para esquerda: deslocamento negativo de X\n";
        std::cout << "Botão esquerdo mais movimento do mouse: rotação do objeto\n";
        std::cout << "Usar scroll do mouse: aplica escala no objeto\n";
    }else{
        if (argc < 2) {
            std::cerr << "Uso: " << argv[0] << " modelo.obj\n";
            return 1;
        }
    
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
        glutInitWindowSize(800, 600);
        glutCreateWindow("Visualizador 3D - Cor por Distância e Direção");
    
        glewInit();
        glEnable(GL_DEPTH_TEST);
    
        compileShaders();
    
        
        loadModel(argv[1]);
        
    
        glutDisplayFunc(display);
        glutKeyboardFunc(keyboard);
        glutSpecialFunc(specialKeys);
        glutMouseFunc(mouse);
        glutMotionFunc(motion);
    
        glutMainLoop();
        return 0;
    }
}