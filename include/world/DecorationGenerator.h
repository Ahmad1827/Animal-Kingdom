#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Decoration {
    std::vector<sf::CircleShape> bushes;
    std::vector<sf::ConvexShape> rocks;
    std::vector<sf::RectangleShape> logs;
};

class DecorationGenerator {
public:
    static Decoration generateDecorations(float startX, float width);
};