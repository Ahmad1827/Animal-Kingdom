#pragma once
#include "entities/Entity.h"

enum class ApeState {
    Airborne,
    Grounded,
    ClimbingTrunk,
    HangingBranch
};

class Ape : public Entity {
private:
    ApeState currentState;
    float gravity;
    float jumpForce;
    float climbSpeed;
    bool droppingThrough;

public:
    Ape(float x, float y);
    void update(float dt) override;
    void processInput();
    ApeState getState() const;
    void setState(ApeState state);
    bool isDroppingThrough() const;
    void setDroppingThrough(bool drop);
    void updateVisuals();
};