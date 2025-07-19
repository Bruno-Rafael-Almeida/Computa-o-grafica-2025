#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// --- Variáveis Globais ---
GLuint phongProgram, basicProgram, textureProgram;
GLuint VAO, VBO;
GLuint textureID;
std::vector<float> vertices;
int drawMode = GL_FILL;
bool usePhongLighting = false;
int textureMappingMode = 0;
glm::vec3 center(0.0f), translation(0.0f);
float scaleFactor = 1.0f;
glm::quat rotationQuat = glm::quat(1, 0, 0, 0);
bool leftMouseDown = false;
glm::vec2 lastMousePos;

glm::vec3 modelMinBounds = glm::vec3(0.0f);
glm::vec3 modelMaxBounds = glm::vec3(0.0f);

//shaders feitos em raw string literal
// Vertex Shader para iluminacao Phong (do modelo 3D)
const char* phongVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
out vec3 fragPos;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    fragPos = vec3(model * vec4(aPos, 1.0));
    normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
)";

// Fragment Shader para iluminacao Phong (do modelo 3D)
const char *phongFragmentShader = R"(
#version 330 core
in vec3 normal; 
in vec3 fragPos;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    float ka = 0.1;
    vec3 ambient = ka * lightColor;

    // Componente Difuso
    float kd = 0.7; // Coeficiente difuso (intensidade da luz difusa)
    vec3 n = normalize(normal);
    vec3 l = normalize(lightPos - fragPos);
    float diff = max(dot(n,l), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    // Componente Especular
    float ks = 0.5; // Coeficiente especular (intensidade da luz especular)
    float shininess = 32.0;
    vec3 v = normalize(viewPos - fragPos);
    vec3 r = reflect(-l, n);
    float spec = pow(max(dot(v, r), 0.0), shininess);
    vec3 specular = ks * spec * lightColor;

    // Combina os componentes e multiplica pela cor do objeto
    vec3 resultLight = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(resultLight, 1.0);
}
)";

// Vertex Shader basico para visualizacao de cor
const char *basicVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormalAsColor; 

out vec3 vertexColor; 

uniform mat4 transform;

void main() {
    vertexColor = aNormalAsColor; 
    gl_Position = transform * vec4(aPos, 1.0); 
}
)";

// Fragment Shader basico para visualizacao de cor
const char *basicFragmentShader = R"(
#version 330 core
in vec3 vertexColor; 
out vec4 FragColor;

void main() {
    // Converte as normais para tons de cinza
    float r = (vertexColor.x + 1.0) / 2.0;
    float g = (vertexColor.y + 1.0) / 2.0;
    float b = (vertexColor.z + 1.0) / 2.0;
    float gray = dot(vec3(r,g,b), vec3(0.299, 0.587, 0.114));
    //float gray = dot(vertexColor, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), 1.0);
}
)";

// Vertex Shader para Textura
// === Vertex Shader para Textura ===
const char* textureVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos; 
layout(location = 1) in vec3 aNormal; 

out vec2 TexCoord; 
uniform mat4 model;      
uniform mat4 view;
uniform mat4 projection;

uniform int textureMappingMode; 

// NOVO: Uniforms para os limites do modelo no espaço do objeto
uniform vec3 modelMinBounds;
uniform vec3 modelMaxBounds;

void main() {
    vec3 objectPos = aPos; 
    
    // Calcula as dimensões do bounding box no espaço do objeto
    vec3 bboxSize = modelMaxBounds - modelMinBounds;
    
    if (textureMappingMode == 1) { // Ortográfica
        // Normaliza as coordenadas X e Y do objeto para o intervalo [0, 1]
        // (objectPos.x - modelMinBounds.x) -> offset para que o mínimo seja 0
        // / bboxSize.x -> divide pelo tamanho para normalizar para [0,1]
        TexCoord.s = (objectPos.x - modelMinBounds.x) / bboxSize.x;
        TexCoord.t = (objectPos.y - modelMinBounds.y) / bboxSize.y;
    } else if (textureMappingMode == 2) { // Cilíndrica
        // s-coordinate: ângulo em torno do eixo Y
        TexCoord.s = atan(objectPos.x - (modelMinBounds.x + modelMaxBounds.x) / 2.0, // Centraliza X no centro do bounding box
                          objectPos.z - (modelMinBounds.z + modelMaxBounds.z) / 2.0) / (2.0 * 3.14159265359) + 0.5;
        // t-coordinate: altura normalizada (Y)
        TexCoord.t = (objectPos.y - modelMinBounds.y) / bboxSize.y;
    } else if (textureMappingMode == 3) { // Esférica
        // Centraliza o ponto do objeto antes de calcular os ângulos para mapeamento esférico
        vec3 centeredObjectPos = objectPos - (modelMinBounds + modelMaxBounds) / 2.0;
        
        TexCoord.s = atan(centeredObjectPos.x, centeredObjectPos.z) / (2.0 * 3.14159265359) + 0.5;
        TexCoord.t = asin(centeredObjectPos.y / length(centeredObjectPos.xyz)) / 3.14159265359 + 0.5;
    }
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment Shader para Textura
const char* textureFragmentShader = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
)";

// Converte coordenadas da tela para coordenadas normalizadas
glm::vec2 getTrackballVector(int x, int y, int width, int height) {
    float nx = (2.0f * x - width) / width;
    float ny = (height - 2.0f * y) / height;
    return glm::vec2(nx, ny);
}

// Carrega um modelo 3D
void loadModel(const std::string& path) {
    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals);

    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Erro ao carregar modelo: " << path << std::endl;
        exit(1);
    }

    const aiMesh* mesh = scene->mMeshes[0];
    vertices.clear();

    aiVector3D minV(1e10f), maxV(-1e10f);

    for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) continue;

        for (int j = 0; j < 3; ++j) {
            unsigned int vertexIndex = face.mIndices[j];

            aiVector3D pos = mesh->mVertices[vertexIndex];
            aiVector3D normal;

            for (unsigned i = 0; i < mesh->mNumFaces; ++i) {
                // ... loop de vértices existente ...
                // Dentro do loop de vértices:
                minV.x = std::min(minV.x, pos.x);
                minV.y = std::min(minV.y, pos.y);
                minV.z = std::min(minV.z, pos.z);

                maxV.x = std::max(maxV.x, pos.x);
                maxV.y = std::max(maxV.y, pos.y);
                maxV.z = std::max(maxV.z, pos.z);
            }

            if (mesh->HasNormals()) {
                normal = mesh->mNormals[vertexIndex];
            } else {
                //calcula normal da face
                aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
                aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
                aiVector3D v2 = mesh->mVertices[face.mIndices[2]];
                normal = (v1 - v0) ^ (v2 - v0); // Produto vetorial para normal da face
                normal.Normalize();
            }

            vertices.push_back(pos.x); // Adiciona coordenadas de posição
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);

            vertices.push_back(normal.x); // Adiciona componentes da normal (usada como normal ou cor)
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // Atualiza min/max para o cálculo do centro e escala
            minV.x = std::min(minV.x, pos.x);
            minV.y = std::min(minV.y, pos.y);
            minV.z = std::min(minV.z, pos.z);

            maxV.x = std::max(maxV.x, pos.x);
            maxV.y = std::max(maxV.y, pos.y);
            maxV.z = std::max(maxV.z, pos.z);
        }
    }
    // Calcula o centro do modelo para centralização
    center = glm::vec3(
        (minV.x + maxV.x) / 2.0f,
        (minV.y + maxV.y) / 2.0f,
        (minV.z + maxV.z) / 2.0f
    );

    // Calcula o fator de escala
    float maxExtent = std::max({ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z });
    scaleFactor = 2.0f / maxExtent;

    // NOVO: Armazenar os limites reais do modelo no espaço do objeto
    modelMinBounds = glm::vec3(minV.x, minV.y, minV.z);
    modelMaxBounds = glm::vec3(maxV.x, maxV.y, maxV.z);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

GLuint createShaderProgram(const char* vsSource, const char* fsSource) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

void loadTexture(const std::string& path) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Descomente e use GL_CLAMP_TO_EDGE para que a textura não se repita
    // Isso garante que a imagem "preencha" a área de 0 a 1 nas coordenadas de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Garante que a textura não se repita na direção S (horizontal)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Garante que a textura não se repita na direção T (vertical)

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Usar mipmaps para minificação
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Filtro linear para magnificação
    glGenerateMipmap(GL_TEXTURE_2D);
    int width, height, nrChannels;
    
    // Carrega a imagem com stb_image
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0); 
    
    if (data) {
        GLenum internalFormat = GL_RGB;
        GLenum dataFormat = GL_RGB;

        if (nrChannels == 4) { // Imagem RGBA
            internalFormat = GL_RGBA;
            dataFormat = GL_RGBA;
        } else if (nrChannels == 1) { // Imagem de 1 canal (tons de cinza)
            internalFormat = GL_RED;   
            dataFormat = GL_RED;       

            // Configurar o swizzling da textura para tons de cinza
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_G, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_A, GL_ONE); 
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D); // Gera mipmaps após carregar a imagem
        std::cout << "Textura " << path << " carregada com sucesso. Dimensões: " << width << "x" << height << ", Canais: " << nrChannels << "\n";
    } else {
        std::cerr << "Falha ao carregar textura: " << path << ". Erro: " << stbi_failure_reason() << std::endl;
    }
    stbi_image_free(data); // Libera a memória da imagem
}

void display() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
    model = model * glm::toMat4(rotationQuat);
    model = glm::scale(model, glm::vec3(scaleFactor));
    model = glm::translate(model, -center);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, -5));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 100.0f);

    glBindVertexArray(VAO);

    if (textureMappingMode != 0) {
        glUseProgram(textureProgram);
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(textureProgram, "ourTexture"), 0);
        glUniform1i(glGetUniformLocation(textureProgram, "textureMappingMode"), textureMappingMode);

        // NOVO: Enviar os limites do bounding box para o shader
        glUniform3f(glGetUniformLocation(textureProgram, "modelMinBounds"), modelMinBounds.x, modelMinBounds.y, modelMinBounds.z);
        glUniform3f(glGetUniformLocation(textureProgram, "modelMaxBounds"), modelMaxBounds.x, modelMaxBounds.y, modelMaxBounds.z);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
    }
    else if (usePhongLighting) {
        glUseProgram(phongProgram);
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(glGetUniformLocation(phongProgram, "lightPos"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(phongProgram, "viewPos"), 0.0f, 0.0f, 5.0f);
        glUniform3f(glGetUniformLocation(phongProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(phongProgram, "objectColor"), 0.1f, 0.5f, 0.8f);
    } else {
        glUseProgram(basicProgram);
        glm::mat4 transform = proj * view * model;
        glUniformMatrix4fv(glGetUniformLocation(basicProgram, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
    }

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int) {
    float step = 0.5f;
    switch (key) {
        case 'v': drawMode = (drawMode == GL_FILL ? GL_LINE : GL_FILL); break;
        case '+': scaleFactor *= 1.1f; break;
        case '-': scaleFactor *= 0.9f; break;
        case '1':
            usePhongLighting = !usePhongLighting;
            if (usePhongLighting) {
                textureMappingMode = 0; // Desativa textura se Phong ativo
            }
            std::cout << "Iluminação Phong: " << (usePhongLighting ? "ATIVADA" : "DESATIVADA") << std::endl;
            break;
        case '2': // Mapeamento Ortográfico
            textureMappingMode = 1;
            usePhongLighting = false;
            std::cout << "Mapeamento de Textura: Ortográfica\n";
            break;
        case '3': // Mapeamento Cilíndrico
            textureMappingMode = 2;
            usePhongLighting = false;
            std::cout << "Mapeamento de Textura: Cilíndrica\n";
            break;
        case '4': // Mapeamento Esférica
            textureMappingMode = 3;
            usePhongLighting = false;
            std::cout << "Mapeamento de Textura: Esférica\n";
            break;
        case '0': // Sem mapeamento de textura (volta para Phong ou Básico)
            textureMappingMode = 0;
            std::cout << "Mapeamento de Textura: DESATIVADO\n";
            break;
        case 27: glutLeaveMainLoop(); break;
        case 'q': case 'Q': glutLeaveMainLoop(); break; 
        case 'w': case 'W': translation.z += step; break;
        case 's': case 'S': translation.z -= step; break;
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
    if (button == GLUT_LEFT_BUTTON) leftMouseDown = (state == GLUT_DOWN);
    if (button == 3) scaleFactor *= 1.1f;
    if (button == 4) scaleFactor *= 0.9f;
    lastMousePos = glm::vec2(x, y);
    glutPostRedisplay();
}

void motion(int x, int y) {
    if (!leftMouseDown) return;
    glm::vec2 cur = getTrackballVector(x, y, 800, 600);
    glm::vec2 prev = getTrackballVector(lastMousePos.x, lastMousePos.y, 800, 600);
    glm::vec3 a = glm::vec3(prev, sqrtf(std::max(0.0f, 1 - glm::dot(prev, prev))));
    glm::vec3 b = glm::vec3(cur, sqrtf(std::max(0.0f, 1 - glm::dot(cur, cur))));
    glm::vec3 axis = glm::cross(a, b);
    float angle = acosf(std::min(1.0f, glm::dot(glm::normalize(a), glm::normalize(b))));
    
    if (glm::length(axis) > 0.0001f && !std::isnan(angle)) {
        rotationQuat = glm::normalize(glm::angleAxis(angle, glm::normalize(axis)) * rotationQuat);
    }
    lastMousePos = glm::vec2(x, y);
    glutPostRedisplay();
}

// --- Função Principal ---
int main(int argc, char** argv) {
    const char *h = "-h";
    if (argc < 2 || (argc == 2 && strcmp(argv[1], h) == 0)) {
        std::cout << "CONTROLES DE MANIPULAÇÃO DA MALHA\n";
        std::cout << "Abertura da malha: ./mesh modelo.obj <textura.png>\n";
        std::cout << "Letra v: alterna entre visualização de faces e wireframe\n";
        std::cout << "Letra 1: alterna entre iluminação Phong e visualização básica\n";
        std::cout << "Letra 2: ativa mapeamento de textura ortográfica\n";
        std::cout << "Letra 3: ativa mapeamento de textura cilíndrica\n";
        std::cout << "Letra 4: ativa mapeamento de textura esférica\n";
        std::cout << "Letra 0: desativa mapeamento de textura\n";
        std::cout << "Setas (cima, baixo, esquerda, direita): deslocamento do objeto\n";
        std::cout << "Botão esquerdo + movimento do mouse: rotação do objeto (trackball)\n";
        std::cout << "Scroll do mouse (+/-): aplica escala no objeto\n";
        std::cout << "ESC: Sair do programa\n";
        return 1;
    }
    
    glutInit(&argc, argv);
    glutInitContextVersion(3, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Visualizador 3D - Iluminacao e Texturas");

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Falha ao inicializar GLEW\n";
        return 1;
    }

    phongProgram = createShaderProgram(phongVertexShader, phongFragmentShader);
    basicProgram = createShaderProgram(basicVertexShader, basicFragmentShader);
    textureProgram = createShaderProgram(textureVertexShader, textureFragmentShader);

    loadModel(argv[1]);
    if (argc > 2) { 
        loadTexture(argv[2]);
    } else {
        std::cout << "Nenhuma textura fornecida.\n";
        std::cout << "Ex: ./mesh modelo.obj textura.png\n";
        
        unsigned char defaultTex[] = {255, 255, 255};
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, defaultTex);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    
    // Limpeza
    glDeleteProgram(phongProgram);
    glDeleteProgram(basicProgram);
    glDeleteProgram(textureProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &textureID);
    
    return 0;
}