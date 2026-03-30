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
	//konstruktor z klasy shader ktory czyta kod z dysku i kompiluje go 
	auto shader = std::make_shared<Shader>(vertexSrc.c_str(), fragmentSrc.c_str());
	//po skompilowaniu wrzucamy do bazy
	Add(name, shader);
	return shader;
}

std::shared_ptr<Shader> ShaderLibrary::Get(const std::string& name)
{
	if (!Exists(name))
	{
		std::cout << "nie znaleziono shadera o nazwie " << name << std::endl;
		return nullptr;
	}

	return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const
{
	//szuka klucza w slowniku i jak dojdzie do konca i nie znajdzie to nie ma
	return m_Shaders.find(name) != m_Shaders.end();
}


