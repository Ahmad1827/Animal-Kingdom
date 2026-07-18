#pragma once
#include <SFML/Graphics.hpp>
#include "entities/Ape.h"

struct CameraTuning {
    sf::Vector2f deadZone;
    float lookAheadMax;
    float lookAheadSpeed;
    float baseLerpX;
    float baseLerpY;
    float fallLerpY;
    float climbLerp;
    float airLerpX;
    float basePreloadMargin;
    float baseUnloadMargin;
};

class CameraManager {
private:
    sf::View view;
    CameraTuning tuning;
    sf::Vector2f anchorPos;
    sf::Vector2f lookAheadOffset;
    sf::Vector2f shakeOffset;
    float shakeTrauma;
    float currentZoom;
    float targetZoom;

public:
    CameraManager(sf::Vector2f size);
    void update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity, ApeState state);
    void addShake(float amount);
    void setZoom(float zoom);
    
    CameraTuning& getTuning();
    const sf::View& getView() const;
    sf::FloatRect getViewBounds() const;
    sf::FloatRect getPreloadBounds(const sf::Vector2f& playerVelocity) const;
    sf::FloatRect getUnloadBounds() const;
};