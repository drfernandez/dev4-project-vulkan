#pragma once

#define GATEWARE_ENABLE_MATH
#include "Gateware.h"

#include "h2bParser.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include "Structures.h"
#include "TextureManager.h"


class LEVELDATA
{
private:
	std::ifstream input;
	TextureManager* texturemanager;

	bool FileExists(std::string file);
	std::string GetFileName(std::string file);
	GW::MATH::GMATRIXF ReadMatrixData();
	bool LoadH2B(const std::string h2bFilePath, H2B::INSTANCED_MESH& instancedMesh);
	unsigned int Find2DMaterialIndex(const H2B::MATERIAL2& material);
	unsigned int Find3DMaterialIndex(const H2B::MATERIAL2& material);
	void LoadMeshFromFile();
	void LoadLightFromFile();
	void LoadCameraFromFile();

public:
	std::string levelName;

	unsigned int numVertices = 0;
	unsigned int numIndices = 0;
	unsigned int numMaterials = 0;

	std::vector<H2B::VERTEX> vertices; 
	std::vector<unsigned int> indices;
	std::vector<H2B::MATERIAL2> materials2D;
	std::vector<H2B::MATERIAL2> materials3D;
	std::map<std::string, H2B::INSTANCED_MESH> uniqueMeshes;
	std::map<std::string, H2B::INSTANCED_MESH> uniqueSkyboxes;
	std::map<std::string, unsigned int> uniqueMaterials2D;
	std::map<std::string, unsigned int> uniqueMaterials3D;
	std::vector<H2B::LIGHT> uniqueLights;
	GW::MATH::GMATRIXF world_camera;


	LEVELDATA();
	~LEVELDATA();
	bool LoadLevel(const std::string& levelFilePath);
	void Clear();
};