/* mesh2.cpp
#include <iostream>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint program, VAO, VBO[3], textureID;
std::vector<float> vertices, colors, texCoords;
int drawMode = GL_FILL;
int textureMode = 2;
std::string currentModelPath;

glm::vec3 center(0.0f), translation(0.0f);
float scaleFactor = 1.0f;
glm::quat rotationQuat = glm::quat(1, 0, 0, 0);

void loadTexture(const std::string& path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!data) {
        std::cerr << "Erro ao carregar textura" << std::endl;
        exit(1);
    }
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    GLenum format = (ch == 3) ? GL_RGB : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
}

void loadModel(const std::string& path) {
    currentModelPath= path;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);
    if (!scene || !scene->HasMeshes()) {
        std::cerr << "Erro ao carregar modelo." << std::endl;
        exit(1);
    }

    vertices.clear(); texCoords.clear();
    const aiMesh* mesh = scene->mMeshes[0];
    aiVector3D minV(1e10), maxV(-1e10);

    for (unsigned i = 0; i < mesh->mNumFaces; i++) {
        const aiFace& face = mesh->mFaces[i];
        if (face.mNumIndices != 3) continue;
        for (int j = 0; j < 3; ++j) {
            aiVector3D v = mesh->mVertices[face.mIndices[j]];
            vertices.push_back(v.x); vertices.push_back(v.y); vertices.push_back(v.z);
            minV.x = std::min(minV.x, v.x); maxV.x = std::max(maxV.x, v.x);
            minV.y = std::min(minV.y, v.y); maxV.y = std::max(maxV.y, v.y);
            minV.z = std::min(minV.z, v.z); maxV.z = std::max(maxV.z, v.z);

            glm::vec2 uv;
            switch (textureMode) {
                case 2: uv = glm::vec2((v.x + 1) / 2.0f, (v.y + 1) / 2.0f); break;
                case 3: uv = glm::vec2(atan2(v.z, v.x) / (2.0f * M_PI) + 0.5f, (v.y + 1) / 2.0f); break;
                case 4: {
                    float r = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
                    uv = glm::vec2(atan2(v.z, v.x) / (2.0f * M_PI) + 0.5f, acos(v.y / r) / M_PI);
                    break;
                }
                default: uv = glm::vec2(0.0f); break;
            }
            texCoords.push_back(uv.x);
            texCoords.push_back(uv.y);
        }
    }

    center = glm::vec3((minV.x + maxV.x) / 2.0f, (minV.y + maxV.y) / 2.0f, (minV.z + maxV.z) / 2.0f);
    float maxExtent = std::max({maxV.x - minV.x, maxV.y - minV.y, maxV.z - minV.z});
    scaleFactor = 2.0f / maxExtent;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float), texCoords.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(1);
}

const char* vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTex;

uniform mat4 transform;
out vec2 TexCoord;

void main() {
    gl_Position = transform * vec4(aPos, 1.0);
    TexCoord = aTex;
})";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
})";

void compileShaders() {
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSrc, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSrc, NULL);
    glCompileShader(fs);
    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
}

void display() {
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.2, 0.2, 0.2, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, drawMode);
    glUseProgram(program);

    glm::mat4 view = glm::translate(glm::mat4(1), glm::vec3(0, 0, -5));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f/600.0f, 0.1f, 100.0f);
    glm::mat4 model = glm::translate(glm::mat4(1), translation);
    model = glm::scale(model * glm::mat4_cast(rotationQuat), glm::vec3(scaleFactor));
    model = glm::translate(model, -center);
    glm::mat4 transform = proj * view * model;
    glUniformMatrix4fv(glGetUniformLocation(program, "transform"), 1, GL_FALSE, glm::value_ptr(transform));

    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

    glutSwapBuffers();
}

void keyboard(unsigned char key, int, int) {
    switch(key) {
        case '1': drawMode = (drawMode == GL_FILL) ? GL_LINE : GL_FILL; break;
        case '2': textureMode = 2; loadModel(currentModelPath); break;
        case '3': textureMode = 3; loadModel(currentModelPath); break;
        case '4': textureMode = 4; loadModel(currentModelPath); break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Uso: ./mesh2 modelo.obj textura.jpg\n";
        return 1;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(800, 600);
    glutCreateWindow("mesh2 - Textura Projeções");
    glewInit();

    compileShaders();
    loadModel(argv[1]);
    loadTexture(argv[2]);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
*/

// mesh2.cpp
// Dependências: GLEW, freeglut, GLM, Assimp, stb_image
/*
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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <algorithm>

// --- Globals ---
GLuint phongProgram, texProgram;
GLuint VAO, VBOpos, VBOuv, VBOnorm;
GLuint textureID;
std::vector<glm::vec3>  positions;
std::vector<glm::vec3>  normals;
std::vector<glm::vec2>  uvs;
bool  usePhong    = false;
int   texMode     = 2;        // 2=ortho,3=cylinder,4=sphere
float angle       = 0.0f;
glm::vec3 center, translation(0.0f);
float scaleFactor = 1.0f;
glm::quat rotationQuat = glm::quat(1, 0, 0, 0);
std::string currentModelPath;

// --- Shader Sources ---

// Phong vertex
const char* phongVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 model, view, proj;
out vec3 FragPos, Normal;
void main(){
    FragPos = vec3(model * vec4(aPos,1));
    Normal  = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = proj * view * vec4(FragPos,1);
}
)";
// Phong fragment
const char* phongFrag = R"(
#version 330 core
in vec3 FragPos, Normal;
out vec4 color;
uniform vec3 lightPos, viewPos, lightColor, objColor;
void main(){
    // ambient
    float ka=0.2;
    vec3 ambient = ka * lightColor;
    // diffuse
    vec3 N = normalize(Normal);
    vec3 L = normalize(lightPos - FragPos);
    float diff = max(dot(N,L),0.0);
    vec3 diffuse = diff * lightColor;
    // specular
    float ks=0.5, shin=32.0;
    vec3 V = normalize(viewPos - FragPos);
    vec3 R = reflect(-L,N);
    float spec = pow(max(dot(V,R),0.0), shin);
    vec3 specular = ks * spec * lightColor;
    vec3 result = (ambient+diffuse+specular) * objColor;
    color = vec4(result,1);
}
)";

// Texture vertex
const char* texVert = R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec2 aUV;
uniform mat4 transform;
out vec2 UV;
void main(){
    UV = aUV;
    gl_Position = transform * vec4(aPos,1);
}
)";
// Texture fragment
const char* texFrag = R"(
#version 330 core
in vec2 UV;
out vec4 color;
uniform sampler2D tex;
void main(){
    color = texture(tex, UV);
}
)";

// --- Helpers ---

GLuint compileProgram(const char* vs,const char* fs){
    GLuint v=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v,1,&vs,nullptr); glCompileShader(v);
    GLuint f=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f,1,&fs,nullptr); glCompileShader(f);
    GLuint p=glCreateProgram();
    glAttachShader(p,v); glAttachShader(p,f);
    glLinkProgram(p);
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

glm::vec2 calcUV(const glm::vec3& v){
    if(texMode==2){
        return {(v.x+1)/2.0f,(v.y+1)/2.0f};
    } else if(texMode==3){
        float u = atan2(v.z,v.x)/(2*M_PI)+0.5f;
        float v_ = (v.y+1)/2.0f;
        return {u,v_};
    } else { // sphere
        float r = length(v);
        float u = atan2(v.z,v.x)/(2*M_PI)+0.5f;
        float v_ = acos(v.y/r)/M_PI;
        return {u,v_};
    }
}

// Recarrega vertices, normais e UV
void loadModel(const std::string& path){
    currentModelPath = path;
    positions.clear(); normals.clear(); uvs.clear();
    Assimp::Importer I;
    const aiScene* S = I.ReadFile(path,
        aiProcess_Triangulate|aiProcess_GenNormals);
    aiMesh* m=S->mMeshes[0];
    aiVector3D minV(1e5f),maxV(-1e5f);
    for(unsigned i=0;i<m->mNumVertices;i++){
        auto &v=m->mVertices[i];
        minV.x= std::min(minV.x,v.x); maxV.x= std::max(maxV.x,v.x);
        minV.y= std::min(minV.y,v.y); maxV.y= std::max(maxV.y,v.y);
        minV.z= std::min(minV.z,v.z); maxV.z= std::max(maxV.z,v.z);
    }
    center = {(minV.x+maxV.x)/2,(minV.y+maxV.y)/2,(minV.z+maxV.z)/2};
    float maxE = std::max({maxV.x-minV.x,maxV.y-minV.y,maxV.z-minV.z});
    scaleFactor=2.0f/maxE;
    for(unsigned f=0;f<m->mNumFaces;f++){
        aiFace& F=m->mFaces[f];
        for(int j=0;j<3;j++){
            unsigned idx=F.mIndices[j];
            aiVector3D v=m->mVertices[idx];
            aiVector3D n=m->mNormals[idx];
            glm::vec3 V{v.x,v.y,v.z}, N{n.x,n.y,n.z};
            positions.push_back(V);
            normals.push_back(normalize(N));
            uvs.push_back(calcUV(V-center)); // UV baseado na posição local
        }
    }
    // Enviar para GPU
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBOpos);
    glBufferData(GL_ARRAY_BUFFER,positions.size()*sizeof(glm::vec3),positions.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,VBOnorm);
    glBufferData(GL_ARRAY_BUFFER,normals.size()*sizeof(glm::vec3),normals.data(),GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER,VBOuv);
    glBufferData(GL_ARRAY_BUFFER,uvs.size()*sizeof(glm::vec2),uvs.data(),GL_STATIC_DRAW);
}

// Carrega textura via stb_image
void loadTexture(const char* path){
    int w,h,c;
    stbi_uc* data=stbi_load(path,&w,&h,&c,0);
    if(!data){std::cerr<<"fail tex\n";exit(1);}
    glGenTextures(1,&textureID);
    glBindTexture(GL_TEXTURE_2D,textureID);
    GLenum fmt=(c==3?GL_RGB:GL_RGBA);
    glTexImage2D(GL_TEXTURE_2D,0,fmt,w,h,0,fmt,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
}

// --- GLUT Callbacks ---

void display(){
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glm::mat4 M = glm::translate(glm::mat4(1),translation)
                * glm::toMat4(rotationQuat)
                * glm::scale(glm::mat4(1),glm::vec3(scaleFactor))
                * glm::translate(glm::mat4(1),-center);
    glm::mat4 V = glm::translate(glm::mat4(1),{0,0,-5});
    glm::mat4 P = glm::perspective(glm::radians(45.0f),800.0f/600.0f,0.1f,100.0f);

    if(usePhong){
        glUseProgram(phongProgram);
        glUniformMatrix4fv(glGetUniformLocation(phongProgram,"model"),1,0,glm::value_ptr(M));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram,"view"),1,0,glm::value_ptr(V));
        glUniformMatrix4fv(glGetUniformLocation(phongProgram,"proj"),1,0,glm::value_ptr(P));
        glUniform3f(glGetUniformLocation(phongProgram,"lightPos"),2,2,2);
        glUniform3f(glGetUniformLocation(phongProgram,"viewPos"),0,0,0);
        glUniform3f(glGetUniformLocation(phongProgram,"lightColor"),1,1,1);
        glUniform3f(glGetUniformLocation(phongProgram,"objColor"),0.8f,0.2f,0.2f);
    } else {
        glUseProgram(texProgram);
        glm::mat4 T = P*V*M;
        glUniformMatrix4fv(glGetUniformLocation(texProgram,"transform"),1,0,glm::value_ptr(T));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,textureID);
        glUniform1i(glGetUniformLocation(texProgram,"tex"),0);
    }

    // Atributos
    glBindVertexArray(VAO);
    // pos
    glBindBuffer(GL_ARRAY_BUFFER,VBOpos);
    glVertexAttribPointer(0,3,GL_FLOAT,0,0,0);
    glEnableVertexAttribArray(0);
    // norm ou uv
    if(usePhong){
        glBindBuffer(GL_ARRAY_BUFFER,VBOnorm);
        glVertexAttribPointer(1,3,GL_FLOAT,0,0,0);
        glEnableVertexAttribArray(1);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER,VBOuv);
        glVertexAttribPointer(1,2,GL_FLOAT,0,0,0);
        glEnableVertexAttribArray(1);
    }

    glDrawArrays(GL_TRIANGLES,0,positions.size());
    glutSwapBuffers();
}

void keyboard(unsigned char k,int x,int y){
    switch(k){
        case '1': usePhong  = true;  break;
        case '2': usePhong  = false; texMode=2; loadModel(currentModelPath); break;
        case '3': usePhong  = false; texMode=3; loadModel(currentModelPath); break;
        case '4': usePhong  = false; texMode=4; loadModel(currentModelPath); break;
        case 27: exit(0);
    }
    glutPostRedisplay();
}

// simples arco-ball omitido por brevidade...
void mouse(int b,int s,int x,int y){}
void motion(int x,int y){}

int main(int c,char**v){
    if(c<3){ std::cerr<<"Uso: "<<v[0]<<" modelo.obj textura.png\n";return 1; }
    glutInit(&c,v);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutCreateWindow("mesh2");
    glewInit();
    // Cria buffers
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBOpos);
    glGenBuffers(1,&VBOnorm);
    glGenBuffers(1,&VBOuv);
    // Compila shaders
    phongProgram = compileProgram(phongVert,phongFrag);
    texProgram   = compileProgram(texVert,texFrag);
    // Carrega recursos
    loadModel(v[1]);
    loadTexture(v[2]);
    // Callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
*/

#include <iostream>
#include <vector>
#include <string>
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
#include "stb_image.h"

GLuint program, VAO, VBO;
int drawMode = GL_FILL;
int mode = 2;

// Transformações
glm::vec3 center;
float minY, maxY;
float scaleFactor;
glm::quat rotationQuat = glm::quat(1,0,0,0);
bool leftDown=false;
glm::vec2 lastMouse;

// Mouse trackball
glm::vec2 screenToNDC(int x,int y){ return {2.0f*x/800-1.0f,1.0f-2.0f*y/600}; }

// Carrega OBJ
std::vector<float> meshData;
void loadModel(const std::string &path){
    Assimp::Importer imp;
    const aiScene* sc = imp.ReadFile(path, aiProcess_Triangulate|aiProcess_GenNormals);
    if(!sc||!sc->HasMeshes()){std::cerr<<"Erro OBJ\n"; exit(1);}    
    const aiMesh* m = sc->mMeshes[0];
    aiVector3D minV(1e10f), maxV(-1e10f);
    meshData.clear();
    for(unsigned i=0;i<m->mNumFaces;i++){
        auto &f=m->mFaces[i];
        for(int j=0;j<3;j++){
            aiVector3D v=m->mVertices[f.mIndices[j]];
            aiVector3D n=m->mNormals[f.mIndices[j]];
            meshData.push_back(v.x);
            meshData.push_back(v.y);
            meshData.push_back(v.z);
            meshData.push_back(n.x);
            meshData.push_back(n.y);
            meshData.push_back(n.z);
            minV.x = std::min(minV.x, v.x);
            minV.y = std::min(minV.y, v.y);
            minV.z = std::min(minV.z, v.z);
            maxV.x = std::max(maxV.x, v.x);
            maxV.y = std::max(maxV.y, v.y);
            maxV.z = std::max(maxV.z, v.z);
        }
    }
    center={(minV.x+maxV.x)/2,(minV.y+maxV.y)/2,(minV.z+maxV.z)/2};
    minY=minV.y; maxY=maxV.y;
    float ext = std::max({maxV.x-minV.x,maxV.y-minV.y,maxV.z-minV.z});
    scaleFactor = 2.0f/ext;
}

// Carrega textura
GLuint tex;
void loadTexture(const char* img){
    int w,h,ch;
    unsigned char *data=stbi_load(img,&w,&h,&ch,3);
    if(!data){std::cerr<<"Erro textura\n";exit(1);}    
    glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D,tex);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
}

// Shaders
const char* vertSrc=R"(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
uniform mat4 transform;
uniform int uMode;
uniform vec3 uCenter;
uniform float uMinY,uMaxY;
out vec3 Normal,FragPos;
out vec2 TexCoord;
void main(){
    vec4 world = transform*vec4(aPos,1);
    FragPos=world.xyz;
    Normal=mat3(transform)*aNormal;
    // UV mapeamento
    if(uMode==2){ // ortográfica XY
        TexCoord = (aPos.xy - uCenter.xy) / (uMaxY - uMinY) + 0.5;
    } else if(uMode==3){ // cilíndrica
        float theta = atan(aPos.z-uCenter.z, aPos.x-uCenter.x);
        TexCoord.s = (theta+3.14159)/6.28318;
        TexCoord.t = (aPos.y - uMinY)/(uMaxY - uMinY);
    } else if(uMode==4){ // esférica
        vec3 p = normalize(aPos - uCenter);
        float th = atan(p.z,p.x);
        float ph = acos(p.y);
        TexCoord.s = (th+3.14159)/6.28318;
        TexCoord.t = ph/3.14159;
    }
    gl_Position=world;
}
)";

const char* fragSrc=R"(
#version 330 core
in vec3 Normal,FragPos;
in vec2 TexCoord;
uniform int uMode;
uniform sampler2D uTex;
out vec4 FragColor;
void main(){
    if(uMode>=2 && uMode<=4) {
        FragColor=texture(uTex,TexCoord);
    } else {
        // modo Phong omitido
        FragColor=vec4(1,1,1,1);
    }
}
)";

GLuint compileProg(){
    auto c=[&](GLenum t,const char*s){GLuint sh=glCreateShader(t);glShaderSource(sh,1,&s,0);glCompileShader(sh);return sh;};
    GLuint vs=c(GL_VERTEX_SHADER,vertSrc),fs=c(GL_FRAGMENT_SHADER,fragSrc);
    GLuint p=glCreateProgram();glAttachShader(p,vs);glAttachShader(p,fs);glLinkProgram(p);
    glDeleteShader(vs);glDeleteShader(fs);return p;
}

/*
void initGL(){
    program=compileProg();
    loadModel("model.obj");
    loadTexture("texture.png");
    glGenVertexArrays(1,&VAO);
    glGenBuffers(1,&VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,meshData.size()*sizeof(float),meshData.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,0,6*sizeof(float),(void*)0);glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,0,6*sizeof(float),(void*)(3*sizeof(float)));glEnableVertexAttribArray(1);
    glEnable(GL_DEPTH_TEST);
}*/

void initGL(){
    program = compileProg();

    // Aqui *não* carregamos mais modelo ou textura — só criamos VAO/VBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // preenchemos o VBO *depois* de chamar loadModel() em main()
    glBufferData(GL_ARRAY_BUFFER,
                 meshData.size() * sizeof(float),
                 meshData.data(),
                 GL_STATIC_DRAW);

    // posição (3 floats) + normal (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float),
                          (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
}


void display(){
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glUseProgram(program);
    // transform
    glm::mat4 model=glm::scale(glm::translate(glm::mat4(1),-center),glm::vec3(scaleFactor));
    glm::mat4 view=glm::translate(glm::mat4(1),glm::vec3(0,0,-5));
    glm::mat4 proj=glm::perspective(glm::radians(45.0f),800/600.0f,0.1f,100.0f);
    glm::mat4 transf=proj*view*model;
    glUniformMatrix4fv(glGetUniformLocation(program,"transform"),1,0,glm::value_ptr(transf));
    glUniform1i(glGetUniformLocation(program,"uMode"),mode);
    glUniform3fv(glGetUniformLocation(program,"uCenter"),1,glm::value_ptr(center));
    glUniform1f(glGetUniformLocation(program,"uMinY"),minY);
    glUniform1f(glGetUniformLocation(program,"uMaxY"),maxY);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D,tex);
    glUniform1i(glGetUniformLocation(program,"uTex"),0);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES,0,meshData.size()/6);
    glutSwapBuffers();
}

void keyboard(unsigned char k,int,int){
    if(k>='1'&&k<='4') mode=k-'0';
    glutPostRedisplay();
}

/*
int main(int c,char**v){
    if(c<3){std::cerr<<"Uso: mesh2 obj texture\n";return 1;}
    glutInit(&c,v);glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(800,600);glutCreateWindow("Mesh2");
    glewInit();
    loadModel(v[1]);
    loadTexture(v[2]);
    initGL();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
}*/

int main(int argc, char** argv){
    if(argc < 3){
        std::cerr<<"Uso: mesh2 <modelo.obj> <textura.png>\n";
        return 1;
    }

    // 1) Inicializa GLUT/GLEW
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(800,600);
    glutCreateWindow("Mesh2 Textured");
    glewInit();

    // 2) Carrega arquivo OBJ e textura *antes* de initGL
    loadModel(argv[1]);
    loadTexture(argv[2]);

    // 3) Agora inicializa VAO/VBO (usa meshData preenchido em loadModel)
    initGL();

    // 4) Callbacks
    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}


