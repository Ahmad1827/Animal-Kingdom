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
    int type; // 0=leaf, 1=firefly, 2=rain, 3=bird, 4=cloud, 5=butterfly
    float phase;
};

class ParticleSystem {
private:
    std::vector<Particle> particles;
    mutable sf::VertexArray mesh; // Mutable to rebuild rendering array in const draw method

    void appendQuad(sf::VertexArray& arr, const sf::Vector2f& pos, float size, sf::Color color) const;

public:
    ParticleSystem();
    void update(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector, float rain, float timeOfDay);
    void draw(sf::RenderWindow& window) const;
    void spawnParticle(const sf::Vector2f& pos, int type, const sf::Vector2f& initialVel = {0,0});
    void spawnImpactLeaves(const sf::Vector2f& pos, int count);
    
    size_t getParticleCount() const;
};