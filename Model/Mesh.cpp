#include "Mesh.h"





Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures)
{
	this->vertices = vertices;
	this->indices = indices;
	this->textures = textures;
	//this->indices.resize(indices.size() / 4);


	setupMesh();
}

void Mesh::setupMesh()
{




	// Create buffers/arrays
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);


	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// load data into vertex buffers
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);


	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	// Setting vertex attribute pointer.


	// Positions

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);
	// Normals

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
	glEnableVertexAttribArray(1);
	// Texture coords


	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
	glEnableVertexAttribArray(2);
	// Tangent

	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
	glEnableVertexAttribArray(3);
	// Bitangent


	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
	glEnableVertexAttribArray(4);
	// Bone ID


	glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void*)(offsetof(Vertex, boneID)));
	glEnableVertexAttribArray(5);

	// Bone Weight

	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(offsetof(Vertex, weight)));
	glEnableVertexAttribArray(6);




	glBindVertexArray(0);

}

void Mesh::Draw(Shader shader)
{


	
	unsigned int diffuseNr = 1;
	unsigned int specularNr = 1;
	unsigned int normalNr = 1;
	unsigned int heightNr = 1;
	//Loop through textures, and bind them where appropriate.
	for (unsigned int i = 0; i < textures.size(); i++)
	{
		glActiveTexture(GL_TEXTURE0 + i); 
		std::string number;
		std::string name = textures[i].type;
		if (name == "texture_diffuse")
			number = std::to_string(diffuseNr++);
		else if (name == "texture_specular")
			number = std::to_string(specularNr++); // Unsigned int -> string
		else if (name == "texture_height")
			number = std::to_string(heightNr++); // Unsigned int -> string

												 // Set the sampler to the correct texture unit
		glUniform1i(glGetUniformLocation(shader.ID, "texture_diffuse1"), i);
		// Then bind the texture.
		glBindTexture(GL_TEXTURE_2D, textures[i].id);
	}

	//Bind VAO and draw
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR)
	{
		std::cout << "GL ERROR! " << err << std::endl;
	}
	// Set back to default
	glActiveTexture(GL_TEXTURE0);

}

void Mesh::DrawShadow(Shader shader)
{
	//Bind VAO and draw
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::Serialize(std::ofstream * stream)
{
	stream->write((char*)vertices.size(), sizeof(unsigned int));
	stream->write((char*)indices.size(), sizeof(unsigned int));
	stream->write((char*)&vertices[0], sizeof(Vertex) * vertices.size());
	stream->write((char*)&indices[0], sizeof(Vertex) * indices.size());
}

void Mesh::FromSerialize(std::ifstream * stream)
{
	unsigned int vsize, isize;
	stream->read((char*)&vsize, sizeof(unsigned int));
	stream->read((char*)&isize, sizeof(unsigned int));

	vertices.resize(vsize);
	indices.resize(isize);
	stream->read((char*)&vertices[0], sizeof(Vertex) * vertices.size());
	stream->read((char*)&indices[0], sizeof(Vertex) * indices.size());
}
