#include "texturemanager.h"


TextureManager::TextureManager()
{
	Clear();
}

TextureManager::~TextureManager()
{
	Clear();
}

TextureManager::TextureManager(const TextureManager& c)
{
	*this = c;
}

TextureManager& TextureManager::operator=(const TextureManager& c)
{
	if (this != &c)
	{
		this->material_2d.resize(c.material_2d.size());
		this->material_3d.resize(c.material_3d.size());
		memcpy_s(this->material_2d.data(), 
			sizeof(H2B::MATERIAL2) * c.material_2d.size(),
			c.material_2d.data(),
			sizeof(H2B::MATERIAL2) * c.material_2d.size());
		memcpy_s(this->material_3d.data(),
			sizeof(H2B::MATERIAL2) * c.material_3d.size(),
			c.material_3d.data(),
			sizeof(H2B::MATERIAL2) * c.material_3d.size());
	}
	return *this;
}

void TextureManager::Initialize()
{
	Clear();
}

void TextureManager::Shutdown()
{
	Clear();
}

unsigned int TextureManager::GetTextureID_2D(const H2B::MATERIAL2& mat)
{
	unsigned int index = -1;
	auto iterator = texture_2d.find(mat.name);
	if (iterator == texture_2d.end())
	{
		index = texture_2d.size();
		texture_2d[mat.name] = index;
		material_2d.push_back(mat);
	}
	else
	{
		index = iterator->second;
	}
	return index;
}

unsigned int TextureManager::GetTextureID_3D(const H2B::MATERIAL2& mat)
{
	unsigned int index = -1;
	auto iterator = texture_3d.find(mat.name);
	if (iterator == texture_3d.end())
	{
		index = texture_3d.size();
		texture_3d[mat.name] = index;
		material_3d.push_back(mat);
	}
	else
	{
		index = iterator->second;
	}
	return index;
}

void TextureManager::Clear()
{
	texture_2d.clear();
	texture_3d.clear();
	material_2d.clear();
	material_3d.clear();
}
