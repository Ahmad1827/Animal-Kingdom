#include "entities/Entity.h"

Entity::Entity(float x, float y, float w, float h, sf::Color color) {
    position = {x, y};
    shape.setSize({w, h});
    shape.setFillColor(color);
    shape.setPosition(position);
}

void Entity::draw(sf::RenderWindow& window) {
    window.draw(shape);
}

sf::Vector2f Entity::getPosition() const {
    return position;
}

void Entity::setPosition(float x, float y) {
    position.x = x;
    position.y = y;
    shape.setPosition(position);
}

sf::FloatRect Entity::getBounds() const {
    return shape.getGlobalBounds();
}

sf::Vector2f Entity::getVelocity() const {
    return velocity;
}

void Entity::setVelocity(float vx, float vy) {
    velocity.x = vx;
    velocity.y = vy;
}

void Entity::setColor(sf::Color color) {
    shape.setFillColor(color);
}