#pragma once
#include <map>
#include <vector>
#include <string>
#include "h2bParser.h"
#include "Structures.h"

class TextureManager
{
public:
	inline static TextureManager* GetInstance() { static TextureManager instance; return &instance; }
	void Initialize();
	void Shutdown();
	unsigned int GetTextureID_2D(const H2B::MATERIAL2& mat);
	unsigned int GetTextureID_3D(const H2B::MATERIAL2& mat);

private:
	TextureManager();
	~TextureManager();
	TextureManager(const TextureManager& c);
	TextureManager& operator=(const TextureManager& c);

	std::map<std::string, unsigned int> texture_2d;
	std::map<std::string, unsigned int> texture_3d;
	std::vector<H2B::MATERIAL2> material_2d;
	std::vector<H2B::MATERIAL2> material_3d;

	void Clear();

};


