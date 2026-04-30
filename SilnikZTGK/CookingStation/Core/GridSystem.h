#pragma once
#include <glm/glm.hpp>
#include <cmath>

// Odpowiada za logike siatki - przeliczanie pozycji swiata na kafelek i z powrotem.
// Nie trzyma stanu - to czysty zestaw funkcji matematycznych.
class GridSystem
{
public:
    // Rozmiar jednego kafelka w jednostkach swiata (dopasuj do swoich modeli tasmy)
    static float CELL_SIZE;

    // Przelicza dowolna pozycje w swiecie na srodek najblizszego kafelka
    static glm::vec3 SnapToGrid(const glm::vec3& worldPos)
    {
        // Snap do CENTRUM kafelka, nie do jego rogu
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