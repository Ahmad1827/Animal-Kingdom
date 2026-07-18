#pragma once
#include <SFML/Graphics.hpp>
#include "entities/Tree.h"
#include "world/DecorationGenerator.h"

class Chunk {
private:
    float startX;
    float width;
    sf::VertexArray terrainMesh;
    std::vector<Tree> trees;
    Decoration decorations;

public:
    Chunk(float startX, float width);
    void draw(sf::RenderWindow& window) const;
    const std::vector<Tree>& getTrees() const;
};