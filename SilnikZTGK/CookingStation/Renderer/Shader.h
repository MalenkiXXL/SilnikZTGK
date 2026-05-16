#ifndef SHADER_H
#define SHADER_H

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector> // <-- DODANE DO OBS£UGI TABLIC MACIERZY

class Shader
{
public:
    // ID gotowego programu cieniuj¹cego
    unsigned int ID;

    // konstruktor czyta i buduje shader na podstawie œcie¿ek do plików
    Shader(const char* vertexPath, const char* fragmentPath)
    {
        // 1. Pobieranie kodu Ÿród³owego z plików
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        // rzucanie wyj¹tków przy problemach z plikami
        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // Otwórz pliki
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;
            // Wczytaj zawartoœæ pliku do strumieni
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // Zamknij pliki
            vShaderFile.close();
            fShaderFile.close();
            // Skonwertuj strumienie na stringi
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        }
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        // 2. Kompilacja shaderów
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // Program cieniuj¹cy
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // Usuñ shadery, bo s¹ ju¿ w³¹czone w program i nie bêd¹ potrzebne
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    // Aktywacja shadera
    void use()
    {
        glUseProgram(ID);
    }

    // Funkcje u³atwiaj¹ce wysy³anie danych uniform
    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void SetBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    // ===========================================================================
    // --- NOWA FUNKCJA: WYSY£ANIE TABLICY MACIERZY W JEDNYM WYO£ANIU (OPTYMALIZACJA FPS) ---
    // ===========================================================================
    void setMat4Array(const std::string& name, const std::vector<glm::mat4>& matrices) const
    {
        if (matrices.empty()) return;
        // Wysy³amy ca³¹ zawartoœæ wektora jednym zapytaniem unikaj¹c pêtli
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), (GLsizei)matrices.size(), GL_FALSE, glm::value_ptr(matrices[0]));
    }

private:
    // Prywatna funkcja pomocnicza do wy³apywania b³êdów kompilacji
    void checkCompileErrors(unsigned int shader, std::string type)
    {
        int success;
        char infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

#endif