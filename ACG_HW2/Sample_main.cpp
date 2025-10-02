#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>

#define WINDOWS
#ifdef WINDOWS 
#include <GL/glew.h>
#else
#include <OpenGL/gl3.h>
#endif
#include <GL/glut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// stb_image png 라이브러리
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// Material 구조체 정의, mtl에서 가져옴
struct Material {
    string name;
    glm::vec3 ambient;  // Ka - 환경광 (Light 없어서 안 쓰임)
    glm::vec3 diffuse;  // Kd (주요 색상)
    glm::vec3 specular; // Ks - 반사광 (Light 없어서 안 쓰임)
    float shininess;    // Ns - 반짝임 정도 (Light 없어서 안 쓰임)
    string texture_map; // map_Kd - Diffuse 텍스처 파일 경로
    
    // default 값
    Material() : ambient(0.0f), diffuse(0.8f, 0.8f, 0.8f), specular(0.0f), shininess(50.0f) {}
};

// MTL 파일 파싱을 위한 전역 변수
map<string, Material> materials;

// Cube 모델 데이터
vector<float> cubeVertices;
vector<unsigned int> cubeIndices;
GLuint CubeVertexBuffer = 0;
GLuint CubeIndexBuffer = 0;

// PiggyBank 모델 데이터
vector<float> piggyVertices;
vector<unsigned int> piggyIndices;
GLuint PiggyVertexBuffer = 0;
GLuint PiggyIndexBuffer = 0;

// 텍스처 관련 변수
GLuint piggyTextureID = 0;

// 각 모델의 실제 색상 (MTL에서 로딩, 실패시 Material 기본값 사용)
Material defaultMaterial;
glm::vec3 cubeActualColor = defaultMaterial.diffuse;
glm::vec3 piggyActualColor = defaultMaterial.diffuse;

// 좌표축 데이터
vector<float> axisVertices;
GLuint AxisVertexBuffer = 0;

// 바운딩 박스 버퍼 (물체 마우스 클릭 감지용)
GLuint CubeBBoxVertexBuffer = 0;
GLuint PiggyBBoxVertexBuffer = 0;
vector<float> cubeBBoxVertices;
vector<float> piggyBBoxVertices;

// 바운딩 박스 좌표 (처음 한 번만 계산해서 저장)
glm::vec3 cubeMinBound, cubeMaxBound;
glm::vec3 piggyMinBound, piggyMaxBound;

// 개별 물체 회전
float cubeRotationX = 0.0f;
float cubeRotationY = 0.0f;
float piggyRotationX = 0.0f;
float piggyRotationY = 0.0f;

// 카메라 회전
float cameraRotationX = 63.5f;
float cameraRotationY = 38.5f;
float cameraDistance = 5.0f;

// 카메라 위치
float cameraX = -7.2f;
float cameraY = 20.f;
float cameraZ = 1.3f;

// 현재 선택된 물체 (0: 없음, 1: cube, 2: piggy)
int selectedObject = 0;

// 마우스 변수
bool mouseDown = false;
int lastMouseX = 0;
int lastMouseY = 0;

GLuint programID;

GLuint LoadShaders(const char* vertex_file_path, const char* fragment_file_path)
{
	//create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	//Read the vertex shader code from the file
	string VertexShaderCode;
	ifstream VertexShaderStream(vertex_file_path, ios::in);
	if(VertexShaderStream.is_open())
	{
		string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	//Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	//Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	printf("Vertex shader compile status: %s\n", Result == GL_TRUE ? "SUCCESS" : "FAILED");
	if (InfoLogLength > 0)
	{
		vector<char> VertexShaderErrorMessage(InfoLogLength);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		fprintf(stdout, "Vertex Shader Error: %s\n", &VertexShaderErrorMessage[0]);
	}

	//Read the fragment shader code from the file
	string FragmentShaderCode;
	ifstream FragmentShaderStream(fragment_file_path, ios::in);
	if (FragmentShaderStream.is_open())
	{
		string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	//Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	//Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	printf("Fragment shader compile status: %s\n", Result == GL_TRUE ? "SUCCESS" : "FAILED");
	if (InfoLogLength > 0)
	{
		vector<char> FragmentShaderErrorMessage(InfoLogLength);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		fprintf(stdout, "Fragment Shader Error: %s\n", &FragmentShaderErrorMessage[0]);
	}

	//Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	printf("Program link status: %s\n", Result == GL_TRUE ? "SUCCESS" : "FAILED");
	if (InfoLogLength > 0)
	{
		vector<char> ProgramErrorMessage(InfoLogLength);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		fprintf(stdout, "Program Link Error: %s\n", &ProgramErrorMessage[0]);
	}
	
	printf("Final ProgramID: %d\n", ProgramID);
 
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
 
    return ProgramID;
}

// Texture Loading 함수 [클로드 도움: stb_image 사용법 및 OpenGL 텍스처 설정]
GLuint loadTexture(const char* path) {
    int width, height, channels;
    
    stbi_set_flip_vertically_on_load(true); // OpenGL UV 좌표계에 맞게 위 아래를 뒤집기
    unsigned char* data = stbi_load(path, &width, &height, &channels, 0);
    
    if (!data) {
        printf("Failed to load texture: %s\n", path);
        printf("STB Error: %s\n", stbi_failure_reason());
        return 0;
    }
    
    printf("Loaded texture: %s (%dx%d, %d channels)\n", path, width, height, channels);
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // 텍스처 포맷 결정
    GLenum format;
    if (channels == 1)
        format = GL_RED;
    else if (channels == 3)
        format = GL_RGB;
    else if (channels == 4)
        format = GL_RGBA;
    else {
        printf("Unsupported channel count: %d\n", channels);
        stbi_image_free(data);
        return 0;
    }
    
    // 텍스처 데이터 업로드
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    
    // 텍스처 필터링 설정
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    // 메모리 해제
    stbi_image_free(data);
    
    printf("Texture loaded successfully with ID: %d\n", textureID);
    return textureID;
}

// mtl 파일 파싱
bool loadMTL(const char* path, map<string, Material>& materials) {
    ifstream file(path, ios::in);
    if (!file.is_open()) {
        printf("Failed to open MTL file: %s\n", path);
        return false;
    }
    
    printf("Loading MTL file: %s\n", path);
    
    Material* currentMaterial = nullptr;
    string line;
    int material_count = 0;
    
    while (getline(file, line) && file.good()) {
        if (line.empty() || line[0] == '#') continue;
        
        stringstream ss(line);
        string command;
        ss >> command;
        
        if (command == "newmtl") {
            string materialName;
            ss >> materialName;
            
            Material newMat;
            newMat.name = materialName;
            materials[materialName] = newMat;
            currentMaterial = &materials[materialName];
            material_count++;
            printf("Found material: %s\n", materialName.c_str());
        }
        else if (currentMaterial != nullptr) {
            if (command == "Ka") {
                // Ambient color
                float r, g, b;
                if (ss >> r >> g >> b) {
                    currentMaterial->ambient = glm::vec3(r, g, b);
                }
            }
            else if (command == "Kd") {
                // Diffuse color (main color)
                float r, g, b;
                if (ss >> r >> g >> b) {
                    currentMaterial->diffuse = glm::vec3(r, g, b);
                    printf("Material %s diffuse: (%.3f, %.3f, %.3f)\n", 
                           currentMaterial->name.c_str(), r, g, b);
                }
            }
            else if (command == "Ks") {
                // Specular color
                float r, g, b;
                if (ss >> r >> g >> b) {
                    currentMaterial->specular = glm::vec3(r, g, b);
                }
            }
            else if (command == "Ns") {
                // Shininess
                float ns;
                if (ss >> ns) {
                    currentMaterial->shininess = ns;
                }
            }
            else if (command == "map_Kd") {
                // Texture map
                string texturePath;
                ss >> texturePath;
                currentMaterial->texture_map = texturePath;
                printf("Material %s texture: %s\n", 
                       currentMaterial->name.c_str(), texturePath.c_str());
            }
        }
    }
    
    file.close();
    printf("MTL file loaded: %d materials\n", material_count);
    return true;
}

// OBJ 파일 파싱
bool loadOBJ(const char* path, vector<float>& vertices, vector<unsigned int>& indices, glm::vec3& actualColor, glm::vec3* centerOffset = nullptr) {
    vector<glm::vec3> temp_vertices;
    vector<glm::vec2> temp_texcoords;
    vector<unsigned int> temp_indices;
    
    // 최종 정점 데이터 [클로드 도움: 인터리브 방식 정점 버퍼 구성법] (position + texcoord 인터리브)
    vector<float> final_vertices;
    
    ifstream file(path, ios::in);
    if (!file.is_open()) {
        printf("Failed to open file: %s\n", path);
        return false;
    }
    
    printf("Loading OBJ file: %s\n", path);
    
    string line;
    int vertex_count = 0, texcoord_count = 0, face_count = 0;
    string currentMaterial = "";
    string mtlFile = "";
    
    // 첫 번째: 정점과 텍스처 좌표 데이터 수집 + MTL 파일 로드
    while (getline(file, line) && file.good()) {
        if (line.empty()) continue;
        
        if (line.substr(0, 7) == "mtllib ") {
            // MTL 파일 참조 발견
            mtlFile = line.substr(7);
            // 공백 제거
            mtlFile.erase(mtlFile.find_last_not_of(" \t\r\n") + 1);
            printf("Found MTL reference: %s\n", mtlFile.c_str());
            
            // MTL 파일 로드
            if (loadMTL(mtlFile.c_str(), materials)) {
                printf("Successfully loaded MTL file\n");
            }
        }
        else if (line.substr(0, 7) == "usemtl ") {
            // Material 사용 명령
            currentMaterial = line.substr(7);
            currentMaterial.erase(currentMaterial.find_last_not_of(" \t\r\n") + 1);
            printf("Using material: %s\n", currentMaterial.c_str());
            
            // [클로드 도움: MTL 파일에서 재질 색상을 읽어와서 actualColor에 적용]
            if (!currentMaterial.empty() && materials.find(currentMaterial) != materials.end()) {
                actualColor = materials[currentMaterial].diffuse;
                printf("Loaded material color: (%.3f, %.3f, %.3f) from %s\n", 
                       actualColor.r, actualColor.g, actualColor.b, currentMaterial.c_str());
            }
        }
        else if (line.substr(0, 2) == "v ") {
            // vertex position 파싱하고 저장
            stringstream ss(line.substr(2));
            float x, y, z;
            if (ss >> x >> y >> z) {
                temp_vertices.push_back(glm::vec3(x, y, z));
                vertex_count++;
            }
        }
        else if (line.substr(0, 3) == "vt ") {
            // Parse texture 파싱하고 저장
            stringstream ss(line.substr(3));
            float u, v;
            if (ss >> u >> v) {
                temp_texcoords.push_back(glm::vec2(u, v));
                texcoord_count++;
            }
        }
    }
    
    file.clear();
    file.seekg(0, ios::beg);
    
    printf("Loaded %d vertices, %d texture coordinates\n", vertex_count, texcoord_count);
    
    // 인덱스를 키로 하는 정점 맵 (중복 없게)
    map<pair<int, int>, int> vertex_map;
    int current_vertex_index = 0;
    
    // 두 번째: face 데이터 처리
    while (getline(file, line) && file.good()) {
        if (line.empty()) continue;
        
        if (line.substr(0, 2) == "f ") {
            // face (format: vertex/texture/normal) 파싱
            stringstream ss(line.substr(2));
            vector<int> face_vertex_indices;
            string token;
            
            while (ss >> token) {
                size_t first_slash = token.find('/');
                size_t second_slash = token.find('/', first_slash + 1);
                
                int vertex_idx = -1, texcoord_idx = -1;
                
                if (first_slash != string::npos) {
                    vertex_idx = stoi(token.substr(0, first_slash));
                    
                    if (second_slash != string::npos) {
                        // format: vertex/texture/normal
                        string tex_str = token.substr(first_slash + 1, second_slash - first_slash - 1);
                        if (!tex_str.empty()) {
                            texcoord_idx = stoi(tex_str);
                        }
                    } else {
                        // format: vertex/texture
                        string tex_str = token.substr(first_slash + 1);
                        if (!tex_str.empty()) {
                            texcoord_idx = stoi(tex_str);
                        }
                    }
                } else {
                    // format: vertex only
                    vertex_idx = stoi(token);
                }
                
                // [클로드 도움: OBJ 파일 파싱 시 인덱스 유효성 검증]
                if (vertex_idx < 1 || vertex_idx > vertex_count) {
                    printf("Invalid vertex index: %d (max: %d) in face line: %s\n", vertex_idx, vertex_count, line.c_str());
                    face_vertex_indices.clear();
                    break;
                }
                
                if (texcoord_idx != -1 && (texcoord_idx < 1 || texcoord_idx > texcoord_count)) {
                    printf("Invalid texture coordinate index: %d (max: %d) in face line: %s\n", texcoord_idx, texcoord_count, line.c_str());
                    texcoord_idx = -1; // 잘못된 텍스처 인덱스는 무시
                }
                
                // 정점-텍스처 조합을 키로 사용
                pair<int, int> vertex_key = make_pair(vertex_idx - 1, texcoord_idx - 1);
                
                // 이미 존재하는 정점-텍스처 조합인지 확인
                if (vertex_map.find(vertex_key) == vertex_map.end()) {
                    // 새로운 조합이면 추가
                    vertex_map[vertex_key] = current_vertex_index;
                    
                    // 정점 위치 추가
                    glm::vec3 pos = temp_vertices[vertex_idx - 1];
                    final_vertices.push_back(pos.x);
                    final_vertices.push_back(pos.y);
                    final_vertices.push_back(pos.z);
                    
                    // 텍스처 좌표 추가
                    if (texcoord_idx != -1 && texcoord_idx - 1 < temp_texcoords.size()) {
                        glm::vec2 tex = temp_texcoords[texcoord_idx - 1];
                        final_vertices.push_back(tex.x);
                        final_vertices.push_back(tex.y);
                    } else {
                        // 기본 텍스처 좌표
                        final_vertices.push_back(0.0f);
                        final_vertices.push_back(0.0f);
                    }
                    
                    current_vertex_index++;
                }
                
                face_vertex_indices.push_back(vertex_map[vertex_key]);
            }
            
            // 삼각형 생성
            if (face_vertex_indices.size() >= 3) {
                if (face_vertex_indices.size() == 3) {
                    temp_indices.push_back(face_vertex_indices[0]);
                    temp_indices.push_back(face_vertex_indices[1]);
                    temp_indices.push_back(face_vertex_indices[2]);
                    face_count++;
                }
                else if (face_vertex_indices.size() == 4) {
                    // 두 개의 삼각형으로 분할해서 저장
                    temp_indices.push_back(face_vertex_indices[0]);
                    temp_indices.push_back(face_vertex_indices[1]);
                    temp_indices.push_back(face_vertex_indices[2]);
                    
                    temp_indices.push_back(face_vertex_indices[0]);
                    temp_indices.push_back(face_vertex_indices[2]);
                    temp_indices.push_back(face_vertex_indices[3]);
                    face_count += 2;
                }
                else {
                    // [클로드 도움: 다각형을 삼각형으로 분할하는 방법 (fan triangulation)]
                    for (size_t i = 1; i < face_vertex_indices.size() - 1; i++) {
                        temp_indices.push_back(face_vertex_indices[0]);
                        temp_indices.push_back(face_vertex_indices[i]);
                        temp_indices.push_back(face_vertex_indices[i + 1]);
                        face_count++;
                    }
                }
            }
        }
    }
    
    file.close();
    
    // 바운딩 박스 계산
    if (centerOffset != nullptr && !temp_vertices.empty()) {
        glm::vec3 minBound = temp_vertices[0];
        glm::vec3 maxBound = temp_vertices[0];
        
        for (const auto& vertex : temp_vertices) {
            minBound.x = min(minBound.x, vertex.x);
            minBound.y = min(minBound.y, vertex.y);
            minBound.z = min(minBound.z, vertex.z);
            maxBound.x = max(maxBound.x, vertex.x);
            maxBound.y = max(maxBound.y, vertex.y);
            maxBound.z = max(maxBound.z, vertex.z);
        }
        
        *centerOffset = (minBound + maxBound) * 0.5f;
        printf("Bounding box: min(%.3f, %.3f, %.3f), max(%.3f, %.3f, %.3f)\n", 
               minBound.x, minBound.y, minBound.z, maxBound.x, maxBound.y, maxBound.z);
        printf("Center offset: (%.3f, %.3f, %.3f)\n", 
               centerOffset->x, centerOffset->y, centerOffset->z);
    }
    
    // 결과 저장
    vertices = final_vertices;
    indices = temp_indices;
    
    printf("OBJ file loaded: %d unique vertices (pos+tex), %d triangles, %zu indices\n", 
           current_vertex_index, face_count, indices.size());
    
    return true;
}

// 바운딩 박스 라인 생성 [클로드 도움: 8개 모서리점을 12개 선분으로 연결하고 lines에 넣는 과정]
void createBoundingBoxLines(const glm::vec3& minBound, const glm::vec3& maxBound, vector<float>& lines) {
    lines.clear();
    
    // 8개 모서리 점
    glm::vec3 corners[8] = {
        glm::vec3(minBound.x, minBound.y, minBound.z), // 0
        glm::vec3(maxBound.x, minBound.y, minBound.z), // 1
        glm::vec3(maxBound.x, maxBound.y, minBound.z), // 2
        glm::vec3(minBound.x, maxBound.y, minBound.z), // 3
        glm::vec3(minBound.x, minBound.y, maxBound.z), // 4
        glm::vec3(maxBound.x, minBound.y, maxBound.z), // 5
        glm::vec3(maxBound.x, maxBound.y, maxBound.z), // 6
        glm::vec3(minBound.x, maxBound.y, maxBound.z)  // 7
    };
    
    // 12개 점을 선으로 연결 (각 선은 시작점과 끝점)
    int edges[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0}, // 아래면
        {4,5}, {5,6}, {6,7}, {7,4}, // 위면
        {0,4}, {1,5}, {2,6}, {3,7}  // 수직 엣지
    };
    
    // 
    for (int i = 0; i < 12; i++) {
        // 시작점
        glm::vec3 start = corners[edges[i][0]];
        lines.push_back(start.x); lines.push_back(start.y); lines.push_back(start.z);
        lines.push_back(0.0f); lines.push_back(0.0f); // 텍스처 값 없으므로 마지막 2은 0
        
        // 끝점
        glm::vec3 end = corners[edges[i][1]];
        lines.push_back(end.x); lines.push_back(end.y); lines.push_back(end.z);
        lines.push_back(0.0f); lines.push_back(0.0f); // 텍스처 값 없으므로 마지막 2은 0
    }
}

// 바운딩 박스 계산 함수
void calculateBoundingBox(const vector<float>& vertices, glm::vec3& minBound, glm::vec3& maxBound) {
    if (vertices.empty()) return;
    
    // 첫 번째 정점으로 초기화 (5개씩 묶여있음: x,y,z,u,v)
    minBound = glm::vec3(vertices[0], vertices[1], vertices[2]);
    maxBound = glm::vec3(vertices[0], vertices[1], vertices[2]);
    
    // 모든 정점을 확인하여 최소/최대값 찾기
    for (size_t i = 0; i < vertices.size(); i += 5) {
        glm::vec3 vertex(vertices[i], vertices[i+1], vertices[i+2]);
        
        minBound.x = min(minBound.x, vertex.x);
        minBound.y = min(minBound.y, vertex.y);
        minBound.z = min(minBound.z, vertex.z);
        
        maxBound.x = max(maxBound.x, vertex.x);
        maxBound.y = max(maxBound.y, vertex.y);
        maxBound.z = max(maxBound.z, vertex.z);
    }
}

// 좌표축 생성 함수
void createAxisGeometry() {
    axisVertices.clear();
    
    float axisLength = 40.0f;
    
    // 각 축의 텍스처 좌표는 0 넣기

    // X축 (빨간색) - 원점에서 +X 방향
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); // 시작점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
    axisVertices.push_back(axisLength); axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); // 끝점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
    
    // Y축 (초록색) - 원점에서 +Y 방향
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); // 시작점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
    axisVertices.push_back(0.0f); axisVertices.push_back(axisLength); axisVertices.push_back(0.0f); // 끝점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
    
    // Z축 (파란색) - 원점에서 +Z 방향
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); // 시작점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f); axisVertices.push_back(axisLength); // 끝점
    axisVertices.push_back(0.0f); axisVertices.push_back(0.0f);
}

// Bounding Box 기반 마우스 클릭 감지
int pickObject(int mouseX, int mouseY) {
    // 화면 크기 480x480 기준
    int screenWidth = 480;
    int screenHeight = 480;
    
    // 화면 중심 기준으로 좌표 변환 (-1 ~ 1)
    float normalizedX = (float)(mouseX - screenWidth/2) / (screenWidth/2);
    float normalizedY = (float)(screenHeight/2 - mouseY) / (screenHeight/2);
    
    // [클로드 도움: 마우스 클릭은 2D인데 바운딩 박스는 3D라서 좌표계를 맞추기 위해 3D→2D 변환하는 과정]
    
    // MVP 변환을 통해 물체들의 Bounding Box 계산
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f); // 3D→2D 투영
    glm::mat4 View = glm::mat4(1.0f);
    View = glm::rotate(View, glm::radians(cameraRotationX), glm::vec3(1, 0, 0)); // 카메라 X축 회전
    View = glm::rotate(View, glm::radians(cameraRotationY), glm::vec3(0, 1, 0)); // 카메라 Y축 회전  
    View = glm::translate(View, glm::vec3(-cameraX, -cameraY, -(cameraDistance + cameraZ))); // 카메라 위치
    
    // Cube 바운딩 박스 계산
    glm::mat4 CubeModel = glm::mat4(1.0f);
    CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationX), glm::vec3(1, 0, 0));
    CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationY), glm::vec3(0, 1, 0));
    
    // Cube 바운딩 박스의 8개 모서리 점
    glm::vec3 cubeCorners[8] = {
        glm::vec3(cubeMinBound.x, cubeMinBound.y, cubeMinBound.z), glm::vec3(cubeMaxBound.x, cubeMinBound.y, cubeMinBound.z),
        glm::vec3(cubeMinBound.x, cubeMaxBound.y, cubeMinBound.z), glm::vec3(cubeMaxBound.x, cubeMaxBound.y, cubeMinBound.z),
        glm::vec3(cubeMinBound.x, cubeMinBound.y, cubeMaxBound.z), glm::vec3(cubeMaxBound.x, cubeMinBound.y, cubeMaxBound.z),
        glm::vec3(cubeMinBound.x, cubeMaxBound.y, cubeMaxBound.z), glm::vec3(cubeMaxBound.x, cubeMaxBound.y, cubeMaxBound.z)
    };
    
    // 8개 점을 화면 좌표로 변환하여 2D 바운딩 박스 범위 구하기
    float cubeMinX = 1000, cubeMaxX = -1000, cubeMinY = 1000, cubeMaxY = -1000;
    for (int i = 0; i < 8; i++) {
        glm::vec4 projected = Projection * View * CubeModel * glm::vec4(cubeCorners[i], 1.0f);
        projected /= projected.w;  // 동차좌표를 일반좌표로 변환
        cubeMinX = min(cubeMinX, projected.x);
        cubeMaxX = max(cubeMaxX, projected.x);
        cubeMinY = min(cubeMinY, projected.y);
        cubeMaxY = max(cubeMaxY, projected.y);
    }
    
    // PiggyBank 바운딩 박스 계산 (동일한 과정)
    glm::mat4 PiggyModel = glm::mat4(1.0f);
    PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationX), glm::vec3(1, 0, 0));
    PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationY), glm::vec3(0, 1, 0));
    
    // PiggyBank 바운딩 박스의 8개 모서리 점
    glm::vec3 piggyCorners[8] = {
        glm::vec3(piggyMinBound.x, piggyMinBound.y, piggyMinBound.z), glm::vec3(piggyMaxBound.x, piggyMinBound.y, piggyMinBound.z),
        glm::vec3(piggyMinBound.x, piggyMaxBound.y, piggyMinBound.z), glm::vec3(piggyMaxBound.x, piggyMaxBound.y, piggyMinBound.z),
        glm::vec3(piggyMinBound.x, piggyMinBound.y, piggyMaxBound.z), glm::vec3(piggyMaxBound.x, piggyMinBound.y, piggyMaxBound.z),
        glm::vec3(piggyMinBound.x, piggyMaxBound.y, piggyMaxBound.z), glm::vec3(piggyMaxBound.x, piggyMaxBound.y, piggyMaxBound.z)
    };
    
    // 8개 점을 화면 좌표로 변환하여 2D 바운딩 박스 범위 구하기
    float piggyMinX = 1000, piggyMaxX = -1000, piggyMinY = 1000, piggyMaxY = -1000;
    for (int i = 0; i < 8; i++) {
        glm::vec4 projected = Projection * View * PiggyModel * glm::vec4(piggyCorners[i], 1.0f);
        projected /= projected.w;  // 동차좌표를 일반좌표로 변환
        
        piggyMinX = min(piggyMinX, projected.x);
        piggyMaxX = max(piggyMaxX, projected.x);
        piggyMinY = min(piggyMinY, projected.y);
        piggyMaxY = max(piggyMaxY, projected.y);
    }
    
    // Bounding Box 내부 클릭 검사
    bool inCube = (normalizedX >= cubeMinX && normalizedX <= cubeMaxX && 
                   normalizedY >= cubeMinY && normalizedY <= cubeMaxY);
    bool inPiggy = (normalizedX >= piggyMinX && normalizedX <= piggyMaxX && 
                    normalizedY >= piggyMinY && normalizedY <= piggyMaxY);
    
    // [클로드 도움: 겹치는 경우 깊이로 판단하는 로직]
    if (inCube && inPiggy) {
        // 둘 다 범위 내에 있으면 더 가까운 것 선택 (바운딩 박스의 중심으로 Z depth 비교)
        glm::vec3 cubeCenterPoint = (cubeMinBound + cubeMaxBound) * 0.5f;
        glm::vec3 piggyCenterPoint = (piggyMinBound + piggyMaxBound) * 0.5f;
        
        glm::vec4 cubeCenter = Projection * View * CubeModel * glm::vec4(cubeCenterPoint, 1.0f);
        glm::vec4 piggyCenter = Projection * View * PiggyModel * glm::vec4(piggyCenterPoint, 1.0f);
        if (cubeCenter.z < piggyCenter.z) {  // 더 앞에 있는 것
            printf("Clicked on Cube (closer)\n");
            return 1;
        } else {
            printf("Clicked on Piggy (closer)\n");
            return 2;
        }
    }
    else if (inCube) {
        printf("Clicked on Cube\n");
        return 1;
    }
    else if (inPiggy) {
        printf("Clicked on Piggy\n");
        return 2;
    }
    
    printf("Clicked on empty space\n");
    return 0; // 빈 공간
}

// 키보드 콜백 함수
void keyboard(unsigned char key, int x, int y) {
    float moveSpeed = 0.1f;
	switch (key) {
	case 'w': cameraZ -= moveSpeed; break;  // 앞으로 이동
	case 's': cameraZ += moveSpeed; break;  // 뒤로 이동
	case 'a': cameraX -= moveSpeed; break;  // 왼쪽으로 이동
	case 'd': cameraX += moveSpeed; break;  // 오른쪽으로 이동
	case 'q': cameraY += moveSpeed; break;  // 위로 이동
	case 'e': cameraY -= moveSpeed; break;  // 아래로 이동
	}
	glutPostRedisplay();
}

// 마우스 클릭 콜백
void mouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_DOWN) {
			mouseDown = true;
			lastMouseX = x;
			lastMouseY = y;
			
			// 클릭한 물체 판별
			selectedObject = pickObject(x, y);
			printf("Selected object: %d\n", selectedObject);
		} else {
			mouseDown = false;
			selectedObject = 0; // 마우스를 놓으면 선택 해제
		}
	}
}

// 마우스 드래그 콜백
void mouseMotion(int x, int y) {
	if (mouseDown) {
		float deltaX = (float)(x - lastMouseX);
		float deltaY = (float)(y - lastMouseY);
		
		float rotationSpeed = 0.5f;
		
		if (selectedObject == 1) {
			// Cube 회전
			cubeRotationY += deltaX * rotationSpeed;
			cubeRotationX += deltaY * rotationSpeed;
			printf("Rotating Cube: X=%.1f, Y=%.1f\n", cubeRotationX, cubeRotationY);
		}
		else if (selectedObject == 2) {
			// PiggyBank 회전
			piggyRotationY += deltaX * rotationSpeed;
			piggyRotationX += deltaY * rotationSpeed;
			printf("Rotating Piggy: X=%.1f, Y=%.1f\n", piggyRotationX, piggyRotationY);
		}
		else {
			// 카메라 회전 (빈 공간 클릭 시)
			cameraRotationY += deltaX * rotationSpeed;
			cameraRotationX += deltaY * rotationSpeed;
			printf("Rotating Camera: X=%.1f, Y=%.1f\n", cameraRotationX, cameraRotationY);
		}
		
		lastMouseX = x;
		lastMouseY = y;
		
		glutPostRedisplay();
	}
}

void renderScene(void)
{
	//Clear all pixels
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // GL_DEPTH_BUFFER_BIT로 깊이 버퍼도 클리어
	//Let's draw something here

    // [클로드 추가: 디버깅용 정보 추가해달라고함]
	printf("=== RenderScene Start ===\n");
	printf("Camera position: (%.2f, %.2f, %.2f)\n", cameraX, cameraY, cameraZ);
	printf("Camera rotation: (%.2f, %.2f)\n", cameraRotationX, cameraRotationY);
	printf("Camera distance: %.2f\n", cameraDistance);
	printf("ProgramID: %d\n", programID);

	// MVP 매트릭스 계산
	glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	
	// 카메라 회전 적용된 View 매트릭스
	glm::mat4 View = glm::mat4(1.0f);
	View = glm::rotate(View, glm::radians(cameraRotationX), glm::vec3(1, 0, 0));
	View = glm::rotate(View, glm::radians(cameraRotationY), glm::vec3(0, 1, 0));
	View = glm::translate(View, glm::vec3(-cameraX, -cameraY, -(cameraDistance + cameraZ)));

	// Shader에 MVP 전달
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	printf("Matrix ID: %d\n", MatrixID);

	// Material 색상 uniform 위치 가져오기
	GLuint MaterialColorID = glGetUniformLocation(programID, "materialColor");
	printf("Material Color ID: %d\n", MaterialColorID);
	
	// 텍스처 관련 uniform 위치 가져오기
	GLuint TextureSamplerID = glGetUniformLocation(programID, "textureSampler");
	GLuint UseTextureID = glGetUniformLocation(programID, "useTexture");

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);  // polygon으로 채워서 그리기

	// Cube 그리기
	printf("CubeVertices size: %zu, CubeIndices size: %zu\n", cubeVertices.size(), cubeIndices.size());
	if (!cubeVertices.empty() && !cubeIndices.empty()) {
		// Cube Model 회전적용
		glm::mat4 CubeModel = glm::mat4(1.0f);
		CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationX), glm::vec3(1, 0, 0));
		CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationY), glm::vec3(0, 1, 0));
		glm::mat4 CubeMVP = Projection * View * CubeModel;
		
		// Cube MVP 전달
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &CubeMVP[0][0]);
		
		// Cube 재질 색상 설정 (MTL에서 로딩된 색상)
		printf("Cube color: (%.3f, %.3f, %.3f)\n", cubeActualColor.r, cubeActualColor.g, cubeActualColor.b);
        // [클로드 도움: 색상을 전달하는 방법과 텍스처를 전달하는 방법을 물어보고 아래의 함수를 적용함]
		glUniform3fv(MaterialColorID, 1, &cubeActualColor[0]);
		glUniform1i(UseTextureID, 0); // 텍스처 사용 안함
		
		glBindBuffer(GL_ARRAY_BUFFER, CubeVertexBuffer);
		
		// Position attribute (location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		
		// Texture coordinate attribute (location = 1)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeIndexBuffer);
		glDrawElements(GL_TRIANGLES, (GLsizei)cubeIndices.size(), GL_UNSIGNED_INT, 0);
		printf("Draw call completed\n");
	} else {
		printf("No vertices or indices to draw!\n");
	}

	// PiggyBank OBJ 모델 그리기
	if (!piggyVertices.empty() && !piggyIndices.empty()) {
		// PiggyBank Model 회전적용
		glm::mat4 PiggyModel = glm::mat4(1.0f);
		PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationX), glm::vec3(1, 0, 0));
		PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationY), glm::vec3(0, 1, 0));
		glm::mat4 PiggyMVP = Projection * View * PiggyModel;
		
		// PiggyBank MVP 전달
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &PiggyMVP[0][0]);
		
		// PiggyBank 텍스처 및 재질 설정
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, piggyTextureID);
        // [클로드 도움: 텍스처 설정 관련 함수 물어보고 아래처럼 적용]
		glUniform1i(TextureSamplerID, 0); // 텍스처 유닛 0 사용
		glUniform1i(UseTextureID, 1); // 텍스처 사용
		glUniform3f(MaterialColorID, 1.0f, 1.0f, 1.0f); // 흰색으로 텍스처 원본 색상 유지

		glBindBuffer(GL_ARRAY_BUFFER, PiggyVertexBuffer);
		
		// Position attribute (location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		
		// Texture coordinate attribute (location = 1)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PiggyIndexBuffer);
		glDrawElements(GL_TRIANGLES, (GLsizei)piggyIndices.size(), GL_UNSIGNED_INT, 0);
	} else {
		printf("No piggy vertices or indices to draw!\n");
	}

	// 좌표축 그리기 (고정된 위치)
	if (!axisVertices.empty()) {
		// 좌표축은 Identity 1.0f
		glm::mat4 AxisModel = glm::mat4(1.0f);
		glm::mat4 AxisMVP = Projection * View * AxisModel;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &AxisMVP[0][0]);
		
		glBindBuffer(GL_ARRAY_BUFFER, AxisVertexBuffer);
		
		// Position attribute (location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		
		// Texture coordinate attribute (location = 1)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		
		// 좌표축은 텍스처 사용 안함
		glUniform1i(UseTextureID, 0);
		
		// X축 그리기 (빨간색)
		glUniform3f(MaterialColorID, 1.0f, 0.0f, 0.0f);
		glDrawArrays(GL_LINES, 0, 2);
		
		// Y축 그리기 (초록색)
		glUniform3f(MaterialColorID, 0.0f, 1.0f, 0.0f);
		glDrawArrays(GL_LINES, 2, 2);
		
		// Z축 그리기 (파란색)
		glUniform3f(MaterialColorID, 0.0f, 0.0f, 1.0f);
		glDrawArrays(GL_LINES, 4, 2);
		
		printf("Coordinate axes drawn\n");
	}

	// 바운딩 박스 그리기
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Cube 바운딩 박스 그리기
	if (!cubeBBoxVertices.empty()) {
		glm::mat4 CubeModel = glm::mat4(1.0f);
		CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationX), glm::vec3(1, 0, 0));
		CubeModel = glm::rotate(CubeModel, glm::radians(cubeRotationY), glm::vec3(0, 1, 0));
		glm::mat4 CubeBBoxMVP = Projection * View * CubeModel;
		
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &CubeBBoxMVP[0][0]);
		glUniform1i(UseTextureID, 0); // 텍스처 사용 안함
		glUniform3f(MaterialColorID, 0.0f, 1.0f, 1.0f);
		
		glBindBuffer(GL_ARRAY_BUFFER, CubeBBoxVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_LINES, 0, cubeBBoxVertices.size() / 5);
		printf("Cube bounding box drawn\n");
	}
	
	// Piggy 바운딩 박스 그리기
	if (!piggyBBoxVertices.empty()) {
		glm::mat4 PiggyModel = glm::mat4(1.0f);
		PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationX), glm::vec3(1, 0, 0));
		PiggyModel = glm::rotate(PiggyModel, glm::radians(piggyRotationY), glm::vec3(0, 1, 0));
		glm::mat4 PiggyBBoxMVP = Projection * View * PiggyModel;
		
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &PiggyBBoxMVP[0][0]);
		glUniform1i(UseTextureID, 0); // 텍스처 사용 안함
		glUniform3f(MaterialColorID, 0.0f, 1.0f, 1.0f);
		
		glBindBuffer(GL_ARRAY_BUFFER, PiggyBBoxVertexBuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		glDrawArrays(GL_LINES, 0, piggyBBoxVertices.size() / 5);
		printf("Piggy bounding box drawn\n");
	}

	//Double buffer
	glutSwapBuffers();
}


void init()
{
#ifdef WINDOWS  // 윈도우즈에서 컴파일 할때는 아래를 포함
    //initilize the glew and check the errors.
    
    GLenum res = glewInit();
    if(res != GLEW_OK)
    {
        fprintf(stderr, "Error: '%s' \n", glewGetErrorString(res));
    }
#endif
	//select the background color
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

}


int main(int argc, char **argv)
{
	//init GLUT and create Window
	//initialize the GLUT
	glutInit(&argc, argv);
	//GLUT_DOUBLE enables double buffering (drawing to a background buffer while the other buffer is displayed)
#ifdef WINDOWS  // 윈도우즈에서 컴파일 할때는 아래를 포함
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
#else
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGBA);
#endif
	//These two functions are used to define the position and size of the window. 
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(480, 480);
	//This is used to define the name of the window.
	glutCreateWindow("Simple OpenGL Window");

	//call initization function
	init();

	//1.
	//Generate VAO
	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	glEnableVertexAttribArray(0); // 속성 0번으로 설정

	// 좌표축 생성 및 버퍼 설정
	createAxisGeometry();
	if (!axisVertices.empty()) {
		glGenBuffers(1, &AxisVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, AxisVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(float), &axisVertices[0], GL_STATIC_DRAW);
		printf("Axis buffer created successfully\n");
	}

	// Load Cube OBJ file and setup buffers
	if (loadOBJ("./cube.obj", cubeVertices, cubeIndices, cubeActualColor)) {
		printf("Successfully loaded Cube OBJ file\n");
		printf("Cube: %zu vertices, %zu indices\n", cubeVertices.size(), cubeIndices.size());
		
		// 첫 몇 개 정점 출력
		printf("First few vertices: ");
		for (int i = 0; i < 9 && i < cubeVertices.size(); i += 3) {
			printf("(%.2f,%.2f,%.2f) ", cubeVertices[i], cubeVertices[i+1], cubeVertices[i+2]);
		}
		printf("\n");
		
		// 정점 버퍼 생성
		glGenBuffers(1, &CubeVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, CubeVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(float), &cubeVertices[0],
			GL_STATIC_DRAW);

		// 인덱스 버퍼 생성
		glGenBuffers(1, &CubeIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int),
			&cubeIndices[0], GL_STATIC_DRAW);
		printf("Cube buffers created successfully\n");
		
		// Cube 바운딩 박스 생성 및 저장
		calculateBoundingBox(cubeVertices, cubeMinBound, cubeMaxBound);
		createBoundingBoxLines(cubeMinBound, cubeMaxBound, cubeBBoxVertices);
		
		if (!cubeBBoxVertices.empty()) {
			glGenBuffers(1, &CubeBBoxVertexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, CubeBBoxVertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, cubeBBoxVertices.size() * sizeof(float), &cubeBBoxVertices[0], GL_STATIC_DRAW);
			printf("Cube bounding box buffer created\n");
		}
	}
	else {
		printf("Failed to load Cube OBJ file\n");
	}

	// Load Piggy OBJ file and setup buffers
	if (loadOBJ("./PiggyBank.obj", piggyVertices, piggyIndices, piggyActualColor)) {
		printf("Successfully loaded Piggy OBJ file\n");
		printf("Piggy: %zu vertices, %zu indices\n", piggyVertices.size(), piggyIndices.size());
		
		// 정점 버퍼 생성
		glGenBuffers(1, &PiggyVertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, PiggyVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, piggyVertices.size() * sizeof(float), &piggyVertices[0],
			GL_STATIC_DRAW);

		// 인덱스 버퍼 생성
		glGenBuffers(1, &PiggyIndexBuffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PiggyIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, piggyIndices.size() * sizeof(unsigned int),
			&piggyIndices[0], GL_STATIC_DRAW);
		printf("Piggy buffers created successfully\n");
		
		// Piggy 바운딩 박스 생성 및 저장
		calculateBoundingBox(piggyVertices, piggyMinBound, piggyMaxBound);
		createBoundingBoxLines(piggyMinBound, piggyMaxBound, piggyBBoxVertices);
		
		if (!piggyBBoxVertices.empty()) {
			glGenBuffers(1, &PiggyBBoxVertexBuffer);
			glBindBuffer(GL_ARRAY_BUFFER, PiggyBBoxVertexBuffer);
			glBufferData(GL_ARRAY_BUFFER, piggyBBoxVertices.size() * sizeof(float), &piggyBBoxVertices[0], GL_STATIC_DRAW);
			printf("Piggy bounding box buffer created\n");
		}
		
		// Piggy 텍스처 로딩
		piggyTextureID = loadTexture("./PiggyBankUVTex.png");
		if (piggyTextureID == 0) {
			printf("Failed to load Piggy texture, using default color\n");
		}
	}
	else {
		printf("Failed to load Piggy OBJ file\n");
	}

	//3. 
	programID = LoadShaders("VertexShader.txt", "FragmentShader.txt");
	glUseProgram(programID);

	glutDisplayFunc(renderScene);
	
	glutKeyboardFunc(keyboard);  // 키보드 콜백
	glutMouseFunc(mouse);        // 마우스 클릭 콜백
	glutMotionFunc(mouseMotion); // 마우스 드래그 콜백
	glEnable(GL_DEPTH_TEST); // 깊이 테스트가능

	//enter GLUT event processing cycle
	glutMainLoop();

	// 각 Buffers 정리
	glDeleteBuffers(1, &CubeVertexBuffer);
	glDeleteBuffers(1, &CubeIndexBuffer);
	glDeleteBuffers(1, &PiggyVertexBuffer);
	glDeleteBuffers(1, &PiggyIndexBuffer);
	glDeleteBuffers(1, &AxisVertexBuffer);
	glDeleteBuffers(1, &CubeBBoxVertexBuffer);
	glDeleteBuffers(1, &PiggyBBoxVertexBuffer);

	glDeleteVertexArrays(1, &VertexArrayID);
	
	return 1;
}

