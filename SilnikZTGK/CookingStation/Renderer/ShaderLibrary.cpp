#include "ShaderLibrary.h"
#include <iostream>

void ShaderLibrary::Add(const std::string& name, const std::shared_ptr<Shader>& shader)
{
	if (Exists(name))
	{
		std::cout << "shader juz istnieje" << std::endl;
	}

	m_Shaders[name] = shader;
}

std::shared_ptr<Shader> ShaderLibrary::Load(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
{
	auto shader = std::make_shared<Shader>(vertexSrc.c_str(), fragmentSrc.c_str());
	Add(name, shader);
	return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name)
{
	if (Exists(name))
	{
		std::cout << "shader juz istnieje" << std::endl;
		return nullptr;
	}

	return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const
{
	return m_Shaders.find(name) != m_Shaders.end();
}


