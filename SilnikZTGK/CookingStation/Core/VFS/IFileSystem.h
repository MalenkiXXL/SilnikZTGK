#pragma once
#include <string>
#include <vector>
#include <cstdint>

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    // G³ówna funkcja: podajesz wirtualn¹ œcie¿kê, dostajesz tablicê bajtów.
    // Zwraca pusty wektor, jeœli plik nie istnieje.
    virtual std::vector<uint8_t> ReadFile(const std::string& filepath) = 0;

    // Opcjonalnie: funkcja sprawdzaj¹ca czy plik istnieje
    virtual bool Exists(const std::string& filepath) = 0;
};