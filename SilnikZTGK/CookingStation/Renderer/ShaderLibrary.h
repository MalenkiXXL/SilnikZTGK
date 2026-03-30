#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "Shader.h"

//menadzer shaderow - ladujemy shader raz potem juz stad pobieramy
class ShaderLibrary
{
public:
	//dodaje istnejacy obiekt shadera do naszej bazy
	void Add(const std::string& name, const std::shared_ptr<Shader>& shader);
	
	//czyta pliki z dysku, tworzy nowy shader i dodaje go do bazy
	std::shared_ptr<Shader> Load(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc);
	//pobiera z bazy shader 
	std::shared_ptr<Shader> Get(const std::string& name);

	//sprawdza czy shader istnieje
	bool Exists(const std::string& name) const;

private:
	//"baza danych" - slownik ktory przepisuje tekstowa nazwe do fizycznego obiektu shadera w pamieci
	std::unordered_map<std::string, std::shared_ptr<Shader>> m_Shaders;
};

