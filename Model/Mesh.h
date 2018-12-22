#ifndef MESH_H
#define MESH_H
//#define string std::string
//#define vector std::vector
#include <glm\glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glfw3.h>
#include <string>
#include <vector>
#include <iostream>
#include "../Utility/Shader.h"
#include "AABB.h"
#include <map>
#include "../Utility/Timer.h"


extern glm::vec3 force;
struct Vertex
{
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
	unsigned int boneID[4] = { 0, 0, 0, 0 };
	float weight[4] = { 0, 0, 0, 0 };

};

struct Face
{
	std::vector<unsigned int> vertexIDs;




};

#ifndef TEXTURES
#define TEXTURES
struct Texture
{
	unsigned int id;
	std::string type;
	std::string path;
};
#endif
class Mesh
{
public:
	unsigned int VAO;
	std::string meshname;
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::vector<Texture> textures;
	AABB collider;

	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture>);
	void Draw(Shader shader);
	void DrawShadow(Shader shader);
	void Serialize(std::ofstream * stream);
	void FromSerialize(std::ifstream * stream);
private:
	//Render Data
	unsigned int VBO, EBO;

	void setupMesh();


};


#endif