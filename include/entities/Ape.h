#pragma once
#include <SFML/Graphics.hpp>
#include "graphics/Animator.h"
#include "entities/LandingDetector.h"
#include "simulation/ApeData.h"

enum class ApeState { Grounded, Airborne, ClimbingTrunk, HangingBranch, ClimbingVine, Working };
enum class AnimState { Idle = 0, Walk = 1, Jump = 2, Climb = 3 };

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
    void setDepthLane(sim::DepthLane lane);
    static void setGlobalShadowParams(float shearX, float projY, sf::Color color);
    sim::DepthLane getDepthLane() const { return depthLane; }
    
    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    sf::FloatRect getBounds() const;
    ApeState getState() const;
    bool isDroppingThrough() const;
    
    ImpactLevel registerLanding(float preCollisionVelocityY);
    
    Animator* getAnimator();
    sf::Sprite& getSprite();
    void setGroundY(float gy);
    float getGroundY() const;

private:
    sf::Sprite sprite;
    sf::Texture& texture;
    Animator animator;
    LandingDetector landingDetector;

    float groundY;

    static float globalShadowShearX;
    static float globalShadowProjY;
    static sf::Color globalShadowColor;
    
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
    sim::DepthLane depthLane;

    AnimState currentAnimState;
    float animTimer;
    int currentFrame;
    bool facingRight;

    const int FRAMES_PER_STATE[4] = {8, 8, 8, 5}; 
    const float DURATION_PER_STATE[4] = {0.15f, 0.08f, 0.10f, 0.12f};

    static sf::Texture masterSpriteSheet;
    static bool newTexturesLoaded;
};