#include "world/Decoration.h"

Decoration::Decoration(float x, float y, int type, float scale, sf::Color color) {
    if (type == 0) {
        shape.setPointCount(5);
        shape.setPoint(0, sf::Vector2f(0.f, -10.f));
        shape.setPoint(1, sf::Vector2f(15.f, -5.f));
        shape.setPoint(2, sf::Vector2f(20.f, 10.f));
        shape.setPoint(3, sf::Vector2f(-15.f, 10.f));
        shape.setPoint(4, sf::Vector2f(-20.f, 0.f));
    } else if (type == 1) {
        shape.setPointCount(3);
        shape.setPoint(0, sf::Vector2f(0.f, -20.f));
        shape.setPoint(1, sf::Vector2f(10.f, 0.f));
        shape.setPoint(2, sf::Vector2f(-10.f, 0.f));
    } else {
        shape.setPointCount(4);
        shape.setPoint(0, sf::Vector2f(-15.f, -5.f));
        shape.setPoint(1, sf::Vector2f(15.f, -5.f));
        shape.setPoint(2, sf::Vector2f(15.f, 5.f));
        shape.setPoint(3, sf::Vector2f(-15.f, 5.f));
    }
    
    shape.setFillColor(color);
    shape.setScale(scale, scale);
    shape.setPosition(x, y - (10.f * scale));
    bounds = shape.getGlobalBounds();
}

void Decoration::update(float dt) {
}

void Decoration::draw(sf::RenderWindow& window) const {
    window.draw(shape);
}

sf::FloatRect Decoration::getBounds() const {
    return bounds;
}