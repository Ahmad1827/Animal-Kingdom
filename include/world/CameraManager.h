#pragma once
#include <SFML/Graphics.hpp>

class CameraManager {
private:
    sf::View view;
    float lerpSpeed;
    sf::Vector2f lookAheadOffset;
    sf::Vector2f shakeOffset;
    float shakeTrauma;

public:
    CameraManager(sf::Vector2f size);
    void update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity);
    void addShake(float amount);
    const sf::View& getView() const;
};