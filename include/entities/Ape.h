#pragma once
#include "entities/Entity.h"

enum class ApeState {
    Airborne,
    Grounded,
    ClimbingTrunk,
    HangingBranch,
    ClimbingVine
};

class Ape : public Entity {
private:
    ApeState currentState;
    float gravity;
    float jumpForce;
    float climbSpeed;
    bool droppingThrough;
    
    // Polish Traversal
    float coyoteTimer;
    float coyoteTimeMax;
    float jumpBufferTimer;
    float jumpBufferMax;
    bool spacePressedLastFrame;

public:
    Ape(float x, float y);
    void update(float dt) override;
    void processInput(float dt);
    ApeState getState() const;
    void setState(ApeState state);
    bool isDroppingThrough() const;
    void setDroppingThrough(bool drop);
    void updateVisuals();
    
    void resetCoyoteTime();
};