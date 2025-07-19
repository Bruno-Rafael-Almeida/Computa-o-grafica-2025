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
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Certifique-se que este arquivo está no seu projeto/caminho de inclusão

// --- Variáveis Globais ---
GLuint phongProgram, basicProgram, textureProgram, VAO, VBO; // Adicionado textureProgram
std::vector<float> vertices;
int drawMode = GL_FILL;
bool usePhongLighting = false;

// Novo: Variáveis para controle de textura
GLuint textureID;
int textureMappingMode = 0; // 0: Sem textura, 1: Ortográfica, 2: Cilíndrica, 3: Esférica

glm::vec3 center(0.0f), translation(0.0f);
float scaleFactor = 1.0f;
glm::quat rotationQuat = glm::quat(1, 0, 0, 0);
bool leftMouseDown = false;
glm::vec2 lastMousePos;

// === Vertex Shader Phong ===
const char* phongVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal; // Normal do vértice
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

// === Fragment Shader Phong ===
/*const char *phongFragmentShader = R"(
#version 330 core
in vec3 normal; 
in vec3 fragPos;
out vec4 FragColor;

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;

void main() {
    float ka = 0.05; // Menor luz ambiente
    vec3 ambient = ka * lightColor;

    float kd = 0.8; 
    vec3 n = normalize(normal);
    vec3 l = normalize(lightPos - fragPos);
    float diff = max(dot(n,l), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    float ks = 0.9; // Maior intensidade especular
    float shininess = 128.0; // Maior brilho para o especular
    vec3 v = normalize(viewPos - fragPos);
    vec3 r = reflect(-l, n);
    float spec = pow(max(dot(v, r), 0.0), shininess);
    vec3 specular = ks * spec * lightColor;

    vec3 light = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(light, 1.0);
}
)";*/

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
    float ka = 0.1; // Ajustado para ser menos "cheio" de ambiente
    vec3 ambient = ka * lightColor;

    float kd = 0.7; // Ajustado
    vec3 n = normalize(normal);
    vec3 l = normalize(lightPos - fragPos);
    float diff = max(dot(n,l), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    float ks = 0.5; // Ajustado
    float shininess = 32.0; // Brilho maior para o especular
    vec3 v = normalize(viewPos - fragPos);
    vec3 r = reflect(-l, n);
    float spec = pow(max(dot(v, r), 0.0), shininess);
    vec3 specular = ks * spec * lightColor;

    vec3 light = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(light, 1.0);
}
)";

// === Vertex Shader Básico (sem iluminação, para cor original) ===
const char *basicVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormalAsColor; // Usaremos a normal como "cor" para este shader

out vec3 vertexColor; // Para passar a "cor" ao fragment shader

uniform mat4 transform;

void main() {
    vertexColor = aNormalAsColor; // Passa a normal (que é usada como cor no modo básico)
    gl_Position = transform * vec4(aPos, 1.0);
}
)";

// === Fragment Shader Básico ===
const char *basicFragmentShader = R"(
#version 330 core
in vec3 vertexColor; // Recebe a "cor" (normal mapeada)
out vec4 FragColor;

void main() {
    // Mapeamento original: normal para cor [0,1], depois grayscale
    float gray = dot(vertexColor, vec3(0.299, 0.587, 0.114));
    FragColor = vec4(vec3(gray), 1.0);
}
)";


// === Vertex Shader para Textura ===
const char* textureVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos; // Posição do vértice no espaço do OBJ/local
layout(location = 1) in vec3 aNormal; // Normal do vértice (ainda pode ser útil, mas não diretamente para TexCoord aqui)

out vec2 TexCoord; // Coordenada de textura a ser interpolada
uniform mat4 model;      // Ainda precisamos da matriz model para o gl_Position
uniform mat4 view;
uniform mat4 projection;

uniform int textureMappingMode; // 1: Ortográfica, 2: Cilíndrica, 3: Esférica

void main() {
    // Usamos aPos (posição no espaço do objeto) para calcular as coordenadas de textura.
    // Isso garante que a textura se mova e gire COM o objeto.
    vec3 objectPos = aPos; 
    
    if (textureMappingMode == 1) { // Ortográfica (Projeção paralela no plano XY do OBJ)
        // Mapeia de acordo com as coordenadas locais do objeto
        TexCoord = objectPos.xy * 0.5 + 0.5; 
    } else if (textureMappingMode == 2) { // Cilíndrica
        // Assumindo cilindro ao longo do eixo Y do OBJ
        TexCoord.s = atan(objectPos.x, objectPos.z) / (2.0 * 3.14159265359) + 0.5;
        TexCoord.t = objectPos.y * 0.5 + 0.5; 
    } else if (textureMappingMode == 3) { // Esférica
        // Assumindo esfera centrada na origem do OBJ
        TexCoord.s = atan(objectPos.x, objectPos.z) / (2.0 * 3.14159265359) + 0.5;
        TexCoord.t = asin(objectPos.y / length(objectPos.xyz)) / 3.14159265359 + 0.5;
    }
    
    // gl_Position ainda é calculada usando a transformação completa (model * aPos)
    // para posicionar o objeto corretamente no mundo.
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";
/*
const char* textureVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos; // Posição do vértice
layout(location = 1) in vec3 aNormal; // Normal do vértice (necessário para cilíndrica/esférica)

out vec2 TexCoord; // Coordenada de textura a ser interpolada
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform int textureMappingMode; // 1: Ortográfica, 2: Cilíndrica, 3: Esférica

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    
    if (textureMappingMode == 1) { // Ortográfica (Projeção paralela no plano XY)
        TexCoord = worldPos.xy * 0.5 + 0.5; // Mapeia de [-1,1] para [0,1]
    } else if (textureMappingMode == 2) { // Cilíndrica
        // Assumindo cilindro ao longo do eixo Y
        // s-coordinate: baseada no ângulo em torno do eixo Y
        // t-coordinate: baseada na altura (coordenada Y)
        TexCoord.s = atan(worldPos.x, worldPos.z) / (2.0 * 3.14159265359) + 0.5;
        TexCoord.t = worldPos.y * 0.5 + 0.5; // Ajuste o fator para o tamanho do seu modelo/textura
    } else if (textureMappingMode == 3) { // Esférica
        // Assumindo esfera centrada na origem
        // s-coordinate: longitude (ângulo em torno do Y)
        // t-coordinate: latitude (ângulo vertical)
        TexCoord.s = atan(worldPos.x, worldPos.z) / (2.0 * 3.14159265359) + 0.5;
        TexCoord.t = asin(worldPos.y / length(worldPos.xyz)) / 3.14159265359 + 0.5;
    }
    
    gl_Position = projection * view * worldPos;
}
)";*/

// === Fragment Shader para Textura ===
const char* textureFragmentShader = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D ourTexture;

void main() {
    FragColor = texture(ourTexture, TexCoord);
}
)";


// --- Funções Auxiliares ---
glm::vec2 getTrackballVector(int x, int y, int width, int height) {
    float nx = (2.0f * x - width) / width;
    float ny = (height - 2.0f * y) / height;
    return glm::vec2(nx, ny);
}

/*
void loadModel(const std::string& path) {
    Assimp::Importer importer;

    // Adicionado aiProcess_GenSmoothNormals para garantir normais para Phong e cálculo de textura
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals); //

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

        for (int j = 0; j < 3; ++j) {
            unsigned int vertexIndex = face.mIndices[j];

            aiVector3D pos = mesh->mVertices[vertexIndex];
            aiVector3D normal;
            if (mesh->HasNormals()) { // Verifica se o modelo tem normais
                normal = mesh->mNormals[vertexIndex]; // Usa a normal do vértice
            } else {
                // Fallback: calcula normal da face (menos preciso para iluminação suave)
                aiVector3D v0 = mesh->mVertices[face.mIndices[0]];
                aiVector3D v1 = mesh->mVertices[face.mIndices[1]];
                aiVector3D v2 = mesh->mVertices[face.mIndices[2]];
                normal = (v1 - v0) ^ (v2 - v0); // Produto vetorial
                normal.Normalize();
            }

            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);

            vertices.push_back(normal.x); // Armazena a normal (usada como normal para Phong, ou como "cor" para basic)
            vertices.push_back(normal.y);
            vertices.push_back(normal.z);

            // Calcula min/max para centralização e escala
            minV.x = std::min(minV.x, pos.x);
            minV.y = std::min(minV.y, pos.y);
            minV.z = std::min(minV.z, pos.z);

            maxV.x = std::max(maxV.x, pos.x);
            maxV.y = std::max(maxV.y, pos.y);
            maxV.z = std::max(maxV.z, pos.z);
        }
    }
    // Calcula o centro e o fator de escala do modelo
    center = glm::vec3(
        (minV.x + maxV.x) / 2.0f,
        (minV.y + maxV.y) / 2.0f,
        (minV.z + maxV.z) / 2.0f
    );
    float maxExtent = std::max({ maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z });
    scaleFactor = 2.0f / maxExtent; // Ajusta a escala para que o modelo caiba na viewport

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    // Layout 0: Posição (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Layout 1: Normal (3 floats) - usada como normal para Phong, ou como "cor" para o basic shader
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Desvincular VAO
}*/

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

// Função auxiliar para compilar shaders e criar um programa
GLuint createShaderProgram(const char* vsSource, const char* fsSource) {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, NULL);
    glCompileShader(vs);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vs, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, NULL);
    glCompileShader(fs);

    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fs, 512, NULL, infoLog);
        std::cerr << "ERRO::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, infoLog);
        std::cerr << "ERRO::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Novo: Função para carregar a textura
void loadTexture(const std::string& path) {
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configurações de wrap e filter
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Evita repetição horizontal
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Evita repetição vertical
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    // Carrega a imagem com stb_image. Assimp não é usado aqui para a imagem.
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    //unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_RGB); // Força 3 canais RGB
    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;
        else if (nrChannels == 1)
            format = GL_RED;
        
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "Textura " << path << " carregada com sucesso. Dimensões: " << width << "x" << height << "\n";
    } else {
        std::cerr << "Falha ao carregar textura: " << path << std::endl;
    }
    stbi_image_free(data); // Libera a memória da imagem
}


// --- Função de Display (Renderização) ---
void display() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glPolygonMode(GL_FRONT_AND_BACK, drawMode);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), translation);
    model = model * glm::toMat4(rotationQuat);
    model = glm::scale(model, glm::vec3(scaleFactor));
    model = glm::translate(model, -center);

    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 5));
    //glm::vec3 cameraPositionWorld = glm::vec3(0.0, 0.0, 5.0); 
    //glUniform3f(glGetUniformLocation(phongProgram, "viewPos"), cameraPositionWorld.x, cameraPositionWorld.y, cameraPositionWorld.z);
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 100.0f);

    glBindVertexArray(VAO);

    if (textureMappingMode != 0) { // Se um modo de textura está ativo (1, 2 ou 3)
        glUseProgram(textureProgram);
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(textureProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(textureProgram, "ourTexture"), 0); // Associa o sampler 'ourTexture' à unidade de textura 0
        glUniform1i(glGetUniformLocation(textureProgram, "textureMappingMode"), textureMappingMode); // Envia o modo de mapeamento

        glActiveTexture(GL_TEXTURE0); // Ativa a unidade de textura 0
        glBindTexture(GL_TEXTURE_2D, textureID); // Vincula a textura
    }
    else if (usePhongLighting) { // Se Phong está ativo (e nenhuma textura)
        glUseProgram(phongProgram);
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram, "projection"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform3f(glGetUniformLocation(phongProgram, "lightPos"), 2.0f, 2.0f, 2.0f);
        glUniform3f(glGetUniformLocation(phongProgram, "viewPos"), 0.0f, 0.0f, 5.0f); // Posição da câmera (assumindo view é apenas translação Z)
        glUniform3f(glGetUniformLocation(phongProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
        glUniform3f(glGetUniformLocation(phongProgram, "objectColor"), 0.1f, 0.5f, 0.8f);
    } else { // Modo básico (cores originais/normais como cor)
        glUseProgram(basicProgram);
        glm::mat4 transform = proj * view * model;
        glUniformMatrix4fv(glGetUniformLocation(basicProgram, "transform"), 1, GL_FALSE, glm::value_ptr(transform));
    }

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
    glutSwapBuffers();
}

// --- Funções de Callback de Entrada ---
void keyboard(unsigned char key, int, int) {
    switch (key) {
        case 'v': drawMode = (drawMode == GL_FILL ? GL_LINE : GL_FILL); break;
        case '+': scaleFactor *= 1.1f; break;
        case '-': scaleFactor *= 0.9f; break;
        case '1': // Alterna Phong
            usePhongLighting = !usePhongLighting;
            if (usePhongLighting) textureMappingMode = 0; // Desativa textura se Phong ativo
            std::cout << "Iluminação Phong: " << (usePhongLighting ? "ATIVADA" : "DESATIVADA") << std::endl;
            break;
        case '2': // Mapeamento Ortográfico
            textureMappingMode = 1;
            usePhongLighting = false; // Desativa Phong se textura ativa
            std::cout << "Mapeamento de Textura: Ortográfica\n";
            break;
        case '3': // Mapeamento Cilíndrico
            textureMappingMode = 2;
            usePhongLighting = false;
            std::cout << "Mapeamento de Textura: Cilíndrica\n";
            break;
        case '4': // Mapeamento Esférico
            textureMappingMode = 3;
            usePhongLighting = false;
            std::cout << "Mapeamento de Textura: Esférica\n";
            break;
        case '0': // Sem mapeamento de textura (volta para Phong ou Básico)
            textureMappingMode = 0;
            std::cout << "Mapeamento de Textura: DESATIVADO\n";
            break;
        case 27: glutLeaveMainLoop(); break; // ESC para sair
        case 'q':
        case 'Q': glutLeaveMainLoop();
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
    if (button == 3) scaleFactor *= 1.1f; // Scroll cima
    if (button == 4) scaleFactor *= 0.9f; // Scroll baixo
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
        std::cout << "Abertura da malha: ./mesh modelo.obj\n";
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
    glutCreateWindow("Visualizador 3D - Iluminação e Texturas");

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Falha ao inicializar GLEW\n";
        return 1;
    }

    phongProgram = createShaderProgram(phongVertexShader, phongFragmentShader);
    basicProgram = createShaderProgram(basicVertexShader, basicFragmentShader);
    textureProgram = createShaderProgram(textureVertexShader, textureFragmentShader); // Novo programa de shader para textura

    loadModel(argv[1]); // Carrega o modelo OBJ
    loadTexture(argv[2]); // Carrega a imagem de textura

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys); // Adicionado de volta para as setas
    glutMouseFunc(mouse);
    glutMotionFunc(motion);

    glutMainLoop();
    
    // Limpeza
    glDeleteProgram(phongProgram);
    glDeleteProgram(basicProgram);
    glDeleteProgram(textureProgram);
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &textureID); // Deletar textura
    
    return 0;
}