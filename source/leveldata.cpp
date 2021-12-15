#include "LevelData.h"

bool LEVELDATA::FileExists(std::string file)
{
	std::ifstream in;
	in.open(file, std::ios_base::in);
	bool result = in.is_open();
	in.close();
	return result;
}

std::string LEVELDATA::GetFileName(const std::string file)
{
	std::string tokenize = file;
	std::string delim = "/\\";
	std::string submit = "";
	size_t first = 0;
	while (first < tokenize.size())
	{
		// find first delimiter
		size_t second = tokenize.find_first_of(delim, first);
		// check to see if it's the end of the string
		if (second == std::string::npos)
		{
			// set the second location to the size of the entire string
			second = tokenize.size();
		}
		// store the value of the tokenized string
		submit = tokenize.substr(first, second - first);
		// adjust the location to search
		first = second + 1;
	}

	// Remove extension
	delim = ".";
	first = submit.find_first_of(delim, 0);
	submit = submit.substr(0, first);
	return submit;
}

GW::MATH::GMATRIXF LEVELDATA::ReadMatrixData()
{
	GW::MATH::GMATRIXF matrix;
	char buffer[256];
	input.getline(buffer, 256, '('); // read up to the start of the matrix data

	input.getline(buffer, 256, ','); // x
	matrix.row1.x = std::stof(buffer);
	input.getline(buffer, 256, ','); // y
	matrix.row1.y = std::stof(buffer);
	input.getline(buffer, 256, ','); // z
	matrix.row1.z = std::stof(buffer);
	input.getline(buffer, 256, ')'); // w
	matrix.row1.w = std::stof(buffer);

	input.getline(buffer, 256);
	input.getline(buffer, 256, '(');

	input.getline(buffer, 256, ','); // x
	matrix.row2.x = std::stof(buffer);
	input.getline(buffer, 256, ','); // y
	matrix.row2.y = std::stof(buffer);
	input.getline(buffer, 256, ','); // z
	matrix.row2.z = std::stof(buffer);
	input.getline(buffer, 256, ')'); // w
	matrix.row2.w = std::stof(buffer);

	input.getline(buffer, 256);
	input.getline(buffer, 256, '(');

	input.getline(buffer, 256, ','); // x
	matrix.row3.x = std::stof(buffer);
	input.getline(buffer, 256, ','); // y
	matrix.row3.y = std::stof(buffer);
	input.getline(buffer, 256, ','); // z
	matrix.row3.z = std::stof(buffer);
	input.getline(buffer, 256, ')'); // w
	matrix.row3.w = std::stof(buffer);

	input.getline(buffer, 256);
	input.getline(buffer, 256, '(');

	input.getline(buffer, 256, ','); // x
	matrix.row4.x = std::stof(buffer);
	input.getline(buffer, 256, ','); // y
	matrix.row4.y = std::stof(buffer);
	input.getline(buffer, 256, ','); // z
	matrix.row4.z = std::stof(buffer);
	input.getline(buffer, 256, ')'); // w
	matrix.row4.w = std::stof(buffer);

	input.getline(buffer, 256);

	return matrix;
}

LEVELDATA::LEVELDATA()
{
	this->Clear();
}

LEVELDATA::~LEVELDATA()
{
	Clear();
}

bool LEVELDATA::LoadLevel(const std::string& filePath)
{
	Clear();

	std::string name = "../levels/" + GetFileName(filePath) + ".txt";

	if (input.is_open())
	{
		input.close();
	}
	input.open(name.c_str(), std::ios_base::in);
	if (!input.is_open())
	{
		return false;
	}
	else
	{
		levelName = GetFileName(filePath);
		while (!input.eof())
		{
			char buffer[256];
			// get the current line from the file
			input.getline(buffer, 256);

			// if the current line is a MESH
			if (strcmp(buffer, "MESH") == 0)
			{
				LoadMeshFromFile();
			}
			// if the current line is a LIGHT
			else if (strcmp(buffer, "LIGHT") == 0)
			{
				LoadLightFromFile();
			}
			// if the current line is a CAMERA
			else if (strcmp(buffer, "CAMERA") == 0)
			{
				LoadCameraFromFile();
			}
		}

		unsigned int meshID = 0;
		unsigned int materialID = 0;

		for (auto& mesh : uniqueMeshes)
		{
			mesh.second.meshID = meshID;
			for (const auto& matrix : mesh.second.matrices)
			{
				meshID++;
			}
		}
		for (auto& skybox : uniqueSkyboxes)
		{
			skybox.second.meshID = meshID;
			for (const auto& matrix : skybox.second.matrices)
			{
				meshID++;
			}
		}

		input.close();
	}
	if (input.is_open())
	{
		input.close();
	}

	return true;
}

bool LEVELDATA::LoadH2B(const std::string h2bFilePath, H2B::INSTANCED_MESH& instancedMesh)
{
	bool success = false;
	H2B::Parser p;
	if (p.Parse(h2bFilePath.c_str()))
	{
		for (const auto& material : p.materials)
		{
			H2B::MATERIAL2 mat = H2B::MATERIAL2(material);
			auto iter = uniqueMaterials2D.find(mat.name);
			if (iter == uniqueMaterials2D.end())
			{
				unsigned int index = uniqueMaterials2D.size();
				uniqueMaterials2D[mat.name] = index;
				materials2D.push_back(mat);
			}
		}
		for (const auto& mesh : p.meshes)
		{
			H2B::MESH2 m = H2B::MESH2(mesh);
			m.drawInfo.indexOffset += numIndices;
			m.materialIndex = FindMaterialIndex(p.materials[m.materialIndex]);
			m.hasColorTexture = (!materials2D[m.materialIndex].map_Kd.empty()) ? 1 : 0;
			instancedMesh.subMeshes.push_back(m);
		}

		instancedMesh.vertexOffset = numVertices;

		for (const auto& vertex : p.vertices)
		{
			vertices.push_back(vertex);
		}
		for (const auto& index : p.indices)
		{
			indices.push_back(index);
		}

		numVertices += p.vertexCount;
		numIndices += p.indexCount;
		numMaterials += p.materialCount;
		success = true;
	}

	return success;
}

unsigned int LEVELDATA::FindMaterialIndex(const H2B::MATERIAL2& material)
{
	unsigned int v = 0;
	auto iter = uniqueMaterials2D.find(material.name);
	if (iter != uniqueMaterials2D.end())
	{
		v = iter->second;
	}
	return v;
}

void LEVELDATA::LoadMeshFromFile()
{
	char buffer[256];
	// get the mesh name
	input.getline(buffer, 256);
	// read matrix data and store
	GW::MATH::GMATRIXF world = ReadMatrixData();

	std::string assetName = GetFileName(buffer);
	std::string path = "../assets/" + assetName + ".h2b";
	auto meshIterator = uniqueMeshes.find(assetName);
	auto skyboxIterator = uniqueSkyboxes.find(assetName);

	H2B::INSTANCED_MESH instancedMesh = {};
	if (FileExists(path) && LoadH2B(path, instancedMesh))
	{
		instancedMesh.meshName = assetName;
		instancedMesh.numInstances = 1;
		instancedMesh.matrices.push_back(world);
	}

	if (assetName == "Skybox")
	{
		if (skyboxIterator == uniqueSkyboxes.end())
		{
			uniqueSkyboxes[assetName] = instancedMesh;
		}
		else
		{
			uniqueSkyboxes[assetName].numInstances += 1;
			uniqueSkyboxes[assetName].matrices.push_back(world);
		}
	}
	else
	{
		if (meshIterator == uniqueMeshes.end())
		{
			uniqueMeshes[assetName] = instancedMesh;
		}
		else
		{
			uniqueMeshes[assetName].numInstances += 1;
			uniqueMeshes[assetName].matrices.push_back(world);
		}
	}
}

void LEVELDATA::LoadLightFromFile()
{
	char buffer[256];
	input.getline(buffer, 256);
	GW::MATH::GMATRIXF information = ReadMatrixData();
	if (strcmp(buffer, "Point") == 0)
	{
		information.row1.w = 1.0f;
	}
	else if (strcmp(buffer, "Spot") == 0)
	{
		information.row1.w = 2.0f;
	}
	H2B::LIGHT l = H2B::LIGHT(
		information.row1,
		information.row2,
		information.row3,
		information.row4);
	uniqueLights.push_back(l);
}

void LEVELDATA::LoadCameraFromFile()
{
	char buffer[256];
	input.getline(buffer, 256);
	GW::MATH::GMATRIXF information = ReadMatrixData();
	world_camera = information;
}

void LEVELDATA::Clear()
{
	if (input.is_open())
	{
		input.close();
	}
	levelName.clear();
	numVertices = 0;
	numIndices = 0;
	numMaterials = 0;
	vertices.clear();
	indices.clear();
	materials2D.clear();
	uniqueSkyboxes.clear();
	uniqueMeshes.clear();
	uniqueMaterials2D.clear();
	uniqueLights.clear();
	world_camera = GW::MATH::GIdentityMatrixF;
}
