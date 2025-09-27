#pragma once

#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Shader.h"
#include "Mesh.h"
#include <string>
#include <fstream>
#include <vector>
using namespace std;

class Model
{
public:
    // model data 
    vector<Mesh>  meshes;

    // constructor, expects a filepath to a 3D model.
    Model(string const& path);

    // draws the model, and thus all its meshes
    void Draw(Shader& shader);

	// returns the number of triangles in the model
	int getTriangles() const;

private:

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void loadModel(string const& path);

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void processNode(aiNode* node, const aiScene* scene);

    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};