#pragma once

#include <vector>
#include <string>

class MeshHandle;
class Animation;

struct aiMesh;
struct aiScene;

struct Vertex;
struct VertexAnim;

class Mesh
{
public:
	static std::vector<MeshHandle> CreateMesh(std::string filename, bool&, bool&);

private:
	static MeshHandle GenHandle_GL(std::vector<unsigned>&, std::vector<Vertex>&);
	static MeshHandle GenHandle_GL(std::vector<unsigned>&, std::vector<VertexAnim>&);

	static void LoadMesh(const aiScene* scene, aiMesh* mesh, std::vector<unsigned>&, std::vector<Vertex>&);
	static void LoadMesh(const aiScene* scene, aiMesh* mesh, std::vector<unsigned>&, std::vector<VertexAnim>&);
};

typedef Mesh* MeshPtr;
