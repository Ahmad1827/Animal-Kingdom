#pragma once
#include <SFML/Graphics.hpp>

class CameraManager {
private:
    sf::View view;
    float lerpSpeed;
    sf::Vector2f lookAheadOffset;
    sf::Vector2f shakeOffset;
    float shakeTrauma;
    float currentZoom;
    float targetZoom;

public:
    CameraManager(sf::Vector2f size);
    void update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity);
    void addShake(float amount);
    void setZoom(float zoom);
    const sf::View& getView() const;
    sf::FloatRect getViewBounds() const;
};