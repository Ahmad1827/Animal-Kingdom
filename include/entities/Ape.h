#pragma once
#include <SFML/Graphics.hpp>
#include "graphics/Animator.h"
#include "entities/LandingDetector.h"
#include "simulation/ApeData.h"

enum class ApeState { Grounded, Airborne, ClimbingTrunk, HangingBranch, ClimbingVine, Working };
enum class AnimState { OldSystem, Idle, Walk, Jump };

class Ape {
public:
    Ape(float x, float y, sf::Texture& texture, bool isPlayer = false);
    
    void update(float dt);
    void draw(sf::RenderTarget& target);
    
    void setPosition(float x, float y);
    void setVelocity(float vx, float vy);
    void setState(ApeState state);
    void setDroppingThrough(bool drop);
    void setIsPlayer(bool player);
    void setCarriedItem(int itemType);
    void setVisualEquipment(sim::ToolType tool, sim::ResourceType res, int amount, bool king);
    
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    sf::FloatRect getBounds() const;
    ApeState getState() const;
    bool isDroppingThrough() const;
    
    ImpactLevel registerLanding(float preCollisionVelocityY);
    
    Animator* getAnimator();
    sf::Sprite& getSprite();

private:
    sf::Sprite sprite;
    sf::Texture& texture;
    Animator animator;
    LandingDetector landingDetector;
    
    sf::FloatRect bounds;
    sf::Vector2f velocity;
    ApeState state;
    bool droppingThrough;
    bool isPlayer;
    int carriedItemType;
    
    sim::ToolType currentTool;
    sim::ResourceType currentResource;
    int resourceAmount;
    bool isKing;

    // New Animation State Tracking
    AnimState currentAnimState;
    float animTimer;
    int currentFrame;
    bool facingRight;

    static sf::Texture texIdle;
    static sf::Texture texWalkRight;
    static sf::Texture texJump;
    static bool newTexturesLoaded;
};