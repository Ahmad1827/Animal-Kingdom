#pragma once
#include "world/WorldObject.h"
#include "graphics/Animator.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include "entities/LandingDetector.h"

enum class ApeState { Airborne, Grounded, ClimbingTrunk, ClimbingVine, HangingBranch };

class Ape {
private:
    sf::FloatRect bounds;
    sf::Vector2f velocity;
    ApeState state;
    bool droppingThrough;
    bool isPlayer;

    sf::Sprite sprite;
    std::unique_ptr<Animator> animator;
    LandingDetector landingDetector;

public:
    Ape(float x, float y, sf::Texture& texture, bool isPlayer = true);
    
    void update(float dt);
    void draw(sf::RenderTarget& target) const;

    const sf::Sprite& getSprite() const { return sprite; }

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    ApeState getState() const;
    bool isDroppingThrough() const;
    ImpactLevel registerLanding(float impactVelocityY);
    void setPosition(float x, float y);
    void setVelocity(float vx, float vy);
    void setState(ApeState newState);
    void setDroppingThrough(bool drop);
    void setIsPlayer(bool player);
    
    Animator* getAnimator() const;
};