#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float life;
    float maxLife;
    sf::Color color;
    float size;
    int type; // 0 = leaf, 1 = firefly, 2 = rain
};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    sf::RectangleShape pShape;

public:
    ParticleSystem();
    void update(float dt, const sf::FloatRect& viewBounds, float wind, float rain, float timeOfDay);
    void draw(sf::RenderWindow& window) const;
    void spawnParticle(const sf::Vector2f& pos, int type);
};