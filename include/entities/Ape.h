#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "graphics/Animator.h"
#include "entities/LandingDetector.h"

enum class ApeState { Grounded, Airborne, ClimbingTrunk, ClimbingVine, HangingBranch };

class Ape {
private:
    sf::FloatRect bounds;
    sf::Sprite sprite;
    std::unique_ptr<Animator> animator;
    sf::Vector2f velocity;
    ApeState state;
    bool droppingThrough;
    bool isPlayer;
    bool facingRight;
    LandingDetector landingDetector;

    // Phase 5 Additions
    int carriedType; // 0=None, 1=Food, 2=Wood, 3=Stone
    sf::RectangleShape carriedBox;

public:
    Ape(float x, float y, sf::Texture& texture, bool isPlayer = false);
    
    void update(float dt);
    void draw(sf::RenderTarget& target) const;
    
    void setPosition(float x, float y);
    void setVelocity(float vx, float vy);
    void setState(ApeState newState);
    void setDroppingThrough(bool drop);
    void setIsPlayer(bool player);
    void setCarriedItem(int type);

    sf::FloatRect getBounds() const;
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    ApeState getState() const;
    bool isDroppingThrough() const;
    ImpactLevel registerLanding(float impactVelocityY);
    
    Animator* getAnimator() const;
    const sf::Sprite& getSprite() const;
};