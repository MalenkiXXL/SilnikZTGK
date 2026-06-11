#pragma once
#include <string>
#include <vector>
#include <cstdint>

class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    virtual std::vector<uint8_t> ReadFile(const std::string& filepath) = 0;

    virtual bool Exists(const std::string& filepath) = 0;
};