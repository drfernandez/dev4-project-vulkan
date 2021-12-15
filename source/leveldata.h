#pragma once

#define GATEWARE_ENABLE_MATH
#include "Gateware.h"

#include "h2bParser.h"
#include <vector>
#include <string>
#include <map>
#include <fstream>

namespace H2B
{
	struct LIGHT
	{
		GW::MATH::GVECTORF position;
		GW::MATH::GVECTORF direction;
		GW::MATH::GVECTORF attributes;
		GW::MATH::GVECTORF color;

		LIGHT()
		{
			position = GW::MATH::GIdentityVectorF;
			direction = GW::MATH::GIdentityVectorF;
			attributes = GW::MATH::GIdentityVectorF;
			color = GW::MATH::GIdentityVectorF;
		}
		LIGHT(GW::MATH::GVECTORF p, GW::MATH::GVECTORF d, GW::MATH::GVECTORF a, GW::MATH::GVECTORF c)
		{
			position = p;
			direction = d;
			attributes = a;
			color = c;
		}
	};
	struct MESH2
	{
		std::string name;
		BATCH drawInfo;
		unsigned int materialIndex;
		unsigned int hasColorTexture;

		MESH2()
		{
			name = "";
			drawInfo = { };
			materialIndex = 0;
			hasColorTexture = 0;
		}
		MESH2(const MESH& m)
		{
			name = (m.name != NULL) ? std::string(m.name) : "";
			drawInfo = m.drawInfo;
			materialIndex = m.materialIndex;
			hasColorTexture = 0;
		}
	};
	struct MATERIAL2
	{
		ATTRIBUTES attrib;
		std::string name;
		std::string map_Kd;
		std::string map_Ks;
		std::string map_Ka;
		std::string map_Ke;
		std::string map_Ns;
		std::string map_d;
		std::string disp;
		std::string decal;
		std::string bump;

		MATERIAL2() 
		{
			attrib = { };
			bump = std::string();
			decal = std::string();
			disp = std::string();
			map_d = std::string();
			map_Ka = std::string();
			map_Kd = std::string();
			map_Ke = std::string();
			map_Ks = std::string();
			map_Ns = std::string();
			name = std::string();
		}
		MATERIAL2(const MATERIAL& m)
		{
			attrib = m.attrib;
			bump = (m.bump != NULL) ? std::string(m.bump) : "";
			decal = (m.decal != NULL) ? std::string(m.decal) : "";
			disp = (m.disp != NULL) ? std::string(m.disp) : "";
			map_d = (m.map_d != NULL) ? std::string(m.map_d) : "";
			map_Ka = (m.map_Ka != NULL) ? std::string(m.map_Ka) : "";
			map_Kd = (m.map_Kd != NULL) ? std::string(m.map_Kd) : "";
			map_Ke = (m.map_Ke != NULL) ? std::string(m.map_Ke) : "";
			map_Ks = (m.map_Ks != NULL) ? std::string(m.map_Ks) : "";
			map_Ns = (m.map_Ns != NULL) ? std::string(m.map_Ns) : "";
			name = (m.name != NULL) ? std::string(m.name) : "";
		}
	};
	struct INSTANCED_MESH
	{
		std::string meshName;
		unsigned int meshID = 0;
		unsigned int numInstances = 0;
		unsigned int vertexOffset = 0;
		std::vector<GW::MATH::GMATRIXF> matrices;
		std::vector<H2B::MESH2> subMeshes;
	};
}

class LEVELDATA
{
private:
	std::ifstream input;

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