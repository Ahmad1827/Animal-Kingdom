#include "world/ParticleSystem.h"
#include <cstdlib>
#include <cmath>
#include <algorithm>

ParticleSystem::ParticleSystem() : mesh(sf::Quads) {}

void ParticleSystem::appendQuad(sf::VertexArray& arr, const sf::Vector2f& pos, float size, sf::Color color) const {
    arr.append(sf::Vertex(pos, color));
    arr.append(sf::Vertex(pos + sf::Vector2f(size, 0.f), color));
    arr.append(sf::Vertex(pos + sf::Vector2f(size, size), color));
    arr.append(sf::Vertex(pos + sf::Vector2f(0.f, size), color));
}

void ParticleSystem::update(float dt, const sf::FloatRect& viewBounds, const sf::Vector2f& windVector, float rain, float timeOfDay) {
    // Ambient Spawning
    if (std::rand() % 100 < 5 + (std::abs(windVector.x) * 50)) { // Leaves
        sf::Vector2f pos(viewBounds.left + (std::rand() % static_cast<int>(viewBounds.width)), viewBounds.top - 50.f + (std::rand() % static_cast<int>(viewBounds.height)));
        spawnParticle(pos, 0); 
    }
    
    if ((timeOfDay > 0.75f || timeOfDay < 0.2f) && std::rand() % 100 < 8) { // Fireflies
        sf::Vector2f pos(viewBounds.left + (std::rand() % static_cast<int>(viewBounds.width)), viewBounds.top + (viewBounds.height * 0.4f) + (std::rand() % static_cast<int>(viewBounds.height * 0.6f)));
        spawnParticle(pos, 1); 
    }

    if (rain > 0.1f && std::rand() % 100 < rain * 100.f) { // Rain
        sf::Vector2f pos(viewBounds.left + (std::rand() % static_cast<int>(viewBounds.width)), viewBounds.top - 100.f);
        spawnParticle(pos, 2);
    }
    
    if (timeOfDay > 0.25f && timeOfDay < 0.7f && std::rand() % 1000 < 2) { // Birds
        sf::Vector2f pos(viewBounds.left + (windVector.x > 0 ? -100.f : viewBounds.width + 100.f), viewBounds.top + (std::rand() % 300));
        spawnParticle(pos, 3, {windVector.x > 0 ? 150.f : -150.f, -10.f});
    }

    // Particle Logic
    for (auto it = particles.begin(); it != particles.end();) {
        it->life -= dt;
        it->phase += dt;
        
        if (it->type == 0) { // Leaf
            it->velocity.x = windVector.x * 300.f + std::sin(it->phase * 5.f) * 30.f;
            it->velocity.y = 40.f + std::cos(it->phase * 3.f) * 20.f;
        } else if (it->type == 1) { // Firefly
            it->velocity.x += (std::rand() % 10 - 5) * dt * 20.f;
            it->velocity.y += (std::rand() % 10 - 5) * dt * 20.f;
            it->color.a = static_cast<sf::Uint8>(150 + std::sin(it->phase * 4.f) * 100);
        } else if (it->type == 2) { // Rain
            it->velocity.x = windVector.x * 400.f;
            it->velocity.y = 800.f + rain * 200.f;
        } else if (it->type == 3) { // Bird
            it->velocity.y = std::sin(it->phase * 10.f) * 20.f; // flapping motion
        }
        
        it->position += it->velocity * dt;

        if (it->life <= 0.f || (it->type != 3 && it->position.y > viewBounds.top + viewBounds.height + 200.f)) {
            it = particles.erase(it);
        } else {
            ++it;
        }
    }
}

void ParticleSystem::draw(sf::RenderWindow& window) const {
    mesh.clear();
    for (const auto& p : particles) {
        if (p.type == 2) { // Rain is drawn as a stretched quad
            sf::VertexArray drop(sf::Lines, 2);
            drop[0].position = p.position; drop[0].color = p.color;
            drop[1].position = p.position - p.velocity * 0.05f; drop[1].color = p.color;
            window.draw(drop);
        } else {
            appendQuad(mesh, p.position, p.size, p.color);
        }
    }
    window.draw(mesh);
}

void ParticleSystem::spawnParticle(const sf::Vector2f& pos, int type, const sf::Vector2f& initialVel) {
    Particle p;
    p.position = pos;
    p.type = type;
    p.velocity = initialVel;
    p.phase = static_cast<float>(std::rand() % 100);

    if (type == 0) { // Leaf
        p.life = 10.f + (std::rand() % 5);
        p.size = 4.f + (std::rand() % 4);
        p.color = sf::Color(30 + (std::rand() % 50), 100 + (std::rand() % 80), 20);
    } else if (type == 1) { // Firefly
        p.life = 5.f + (std::rand() % 5);
        p.size = 3.f;
        p.color = sf::Color(200, 255, 100, 255);
    } else if (type == 2) { // Rain
        p.life = 3.f;
        p.size = 2.f;
        p.color = sf::Color(150, 180, 220, 150);
    } else if (type == 3) { // Bird
        p.life = 20.f;
        p.size = 6.f;
        p.color = sf::Color(10, 10, 15);
    }
    p.maxLife = p.life;
    particles.push_back(p);
}

void ParticleSystem::spawnImpactLeaves(const sf::Vector2f& pos, int count) {
    for (int i = 0; i < count; ++i) {
        sf::Vector2f vel((std::rand() % 200) - 100.f, - (std::rand() % 150));
        spawnParticle(pos, 0, vel);
        particles.back().life = 2.f; // Short burst life
    }
}

size_t ParticleSystem::getParticleCount() const { return particles.size(); }