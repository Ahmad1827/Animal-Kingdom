#pragma once
#include <SFML/Graphics.hpp>

class GameState {
public:
    virtual ~GameState() = default;
    virtual void init() = 0;
    virtual void processEvents(const sf::Event& event) = 0;
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
};