#pragma once
#include <glm/glm.hpp>
#include <cmath>

struct IVec2Hash
{
    std::size_t operator()(const glm::ivec2& v) const
    {
      
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y << 1));
    }
};


class GridSystem
{
public:
    static float CELL_SIZE;

    static glm::vec3 SnapToGrid(const glm::vec3& worldPos)
    {
        int cellX = (int)std::floor(worldPos.x / CELL_SIZE);
        int cellZ = (int)std::floor(worldPos.z / CELL_SIZE);
        float snappedX = (cellX + 0.5f) * CELL_SIZE;
        float snappedZ = (cellZ + 0.5f) * CELL_SIZE;
        return glm::vec3(snappedX, worldPos.y, snappedZ);
    }

    static glm::ivec2 WorldToCell(const glm::vec3& worldPos)
    {
        int cellX = (int)std::floor(worldPos.x / CELL_SIZE);
        int cellZ = (int)std::floor(worldPos.z / CELL_SIZE);
        return glm::ivec2(cellX, cellZ);
    }

    static glm::vec3 CellToWorld(const glm::ivec2& cell, float y = 0.0f)
    {
        float wx = (cell.x + 0.5f) * CELL_SIZE;
        float wz = (cell.y + 0.5f) * CELL_SIZE;
        return glm::vec3(wx, y, wz);
    }
    
};