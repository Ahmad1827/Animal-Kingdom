#include "world/ParticleSystem.h"
#include <cstdlib>
#include <cmath>

ParticleSystem::ParticleSystem() {
    pShape.setOutlineThickness(0.f);
}

void ParticleSystem::update(float dt, const sf::FloatRect& viewBounds, float wind, float rain, float timeOfDay) {
    // Spawn ambient particles naturally around the camera
    if (std::rand() % 100 < 10 + (wind * 20)) {
        sf::Vector2f pos(
            viewBounds.left + (std::rand() % static_cast<int>(viewBounds.width)),
            viewBounds.top - 50.f + (std::rand() % static_cast<int>(viewBounds.height))
        );
        spawnParticle(pos, 0); // Leaf
    }
    
    // Spawn fireflies at night (timeOfDay > 0.7 or < 0.2)
    if ((timeOfDay > 0.75f || timeOfDay < 0.2f) && std::rand() % 100 < 5) {
        sf::Vector2f pos(
            viewBounds.left + (std::rand() % static_cast<int>(viewBounds.width)),
            viewBounds.top + (viewBounds.height / 2.f) + (std::rand() % static_cast<int>(viewBounds.height / 2.f))
        );
        spawnParticle(pos, 1); // Firefly
    }

    for (auto it = particles.begin(); it != particles.end();) {
        it->life -= dt;
        
        if (it->type == 0) { // Leaf
            it->velocity.x = wind * 150.f + std::sin(it->life * 5.f) * 30.f;
            it->velocity.y = 40.f + std::cos(it->life * 3.f) * 20.f;
        } else if (it->type == 1) { // Firefly
            it->velocity.x = std::sin(it->life * 2.f) * 20.f;
            it->velocity.y = std::cos(it->life * 2.5f) * 20.f;
            // Pulse alpha
            it->color.a = static_cast<sf::Uint8>(150 + std::sin(it->life * 4.f) * 100);
        }
        
        it->position += it->velocity * dt;

        if (it->life <= 0.f || it->position.y > viewBounds.top + viewBounds.height + 100.f) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleSystem::draw(sf::RenderWindow& window) const {
    // To respect constness of draw, we copy the shape
    sf::RectangleShape shape = pShape;
    for (const auto& p : particles) {
        shape.setSize({p.size, p.size});
        shape.setPosition(p.position);
        shape.setFillColor(p.color);
        window.draw(shape);
    }
}

void ParticleSystem::spawnParticle(const sf::Vector2f& pos, int type) {
    Particle p;
    p.position = pos;
    p.type = type;
    if (type == 0) { // Leaf
        p.life = 10.f + (std::rand() % 5);
        p.maxLife = p.life;
        p.size = 4.f + (std::rand() % 4);
        p.color = sf::Color(30 + (std::rand() % 50), 100 + (std::rand() % 80), 20);
    } else if (type == 1) { // Firefly
        p.life = 5.f + (std::rand() % 5);
        p.maxLife = p.life;
        p.size = 3.f;
        p.color = sf::Color(200, 255, 100, 255);
    }
    p.velocity = {0.f, 0.f};
    particles.push_back(p);
}