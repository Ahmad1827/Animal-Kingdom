#pragma once
#include <SFML/Graphics.hpp>

class Entity {
protected:
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::RectangleShape shape;
    float speed;

public:
    Entity(float x, float y, float w, float h, sf::Color color);
    virtual ~Entity() = default;
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window);
    sf::Vector2f getPosition() const;
    void setPosition(float x, float y);
    sf::FloatRect getBounds() const;
    sf::Vector2f getVelocity() const;
    void setVelocity(float vx, float vy);
    void setColor(sf::Color color);
};