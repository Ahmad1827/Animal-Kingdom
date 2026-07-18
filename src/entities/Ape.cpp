#include "entities/Ape.h"
#include <algorithm>

Ape::Ape(float x, float y) : Entity(x, y, 28.f, 28.f, sf::Color::Red) {
    speed = 280.f; // Slightly faster, more agile
    climbSpeed = 220.f;
    gravity = 1300.f;
    jumpForce = -580.f;
    currentState = ApeState::Airborne;
    droppingThrough = false;
    
    coyoteTimeMax = 0.12f;
    coyoteTimer = 0.f;
    jumpBufferMax = 0.15f;
    jumpBufferTimer = 0.f;
    spacePressedLastFrame = false;
}

void Ape::processInput(float dt) {
    bool spacePressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
    
    // Jump Buffering
    if (spacePressed && !spacePressedLastFrame) {
        jumpBufferTimer = jumpBufferMax;
    } else {
        jumpBufferTimer -= dt;
    }
    spacePressedLastFrame = spacePressed;

    if (currentState == ApeState::ClimbingTrunk || currentState == ApeState::ClimbingVine) {
        velocity.y = 0.f;
        velocity.x = 0.f;
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            velocity.y = -climbSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            velocity.y = climbSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            setState(ApeState::Airborne);
            velocity.x = -speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            setState(ApeState::Airborne);
            velocity.x = speed;
        }
        if (jumpBufferTimer > 0.f) {
            setState(ApeState::Airborne);
            velocity.y = jumpForce;
            jumpBufferTimer = 0.f;
        }
    } else if (currentState == ApeState::HangingBranch) {
        velocity.y = 0.f;
        velocity.x = 0.f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            velocity.x = -climbSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            velocity.x = climbSpeed;
        }
        if (jumpBufferTimer > 0.f) {
            setState(ApeState::Airborne);
            jumpBufferTimer = 0.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            setState(ApeState::Airborne);
            velocity.y = 100.f;
        }
    } else {
        velocity.x = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            velocity.x = -speed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            velocity.x = speed;
        }
        
        if (currentState == ApeState::Grounded) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                if (jumpBufferTimer > 0.f) {
                    droppingThrough = true;
                    setState(ApeState::Airborne);
                    jumpBufferTimer = 0.f;
                }
            } else if (jumpBufferTimer > 0.f) {
                velocity.y = jumpForce;
                setState(ApeState::Airborne);
                jumpBufferTimer = 0.f;
            }
        } else {
            // Airborne Coyote Time Check
            if (coyoteTimer > 0.f && jumpBufferTimer > 0.f) {
                velocity.y = jumpForce;
                setState(ApeState::Airborne);
                jumpBufferTimer = 0.f;
                coyoteTimer = 0.f;
            }
        }
    }
}

void Ape::update(float dt) {
    coyoteTimer -= dt;
    processInput(dt);
    
    if (currentState == ApeState::Airborne || currentState == ApeState::Grounded) {
        velocity.y += gravity * dt;
        // Cap falling speed
        if (velocity.y > 1000.f) velocity.y = 1000.f; 
    }

    position += velocity * dt;
    shape.setPosition(position);
    updateVisuals();
}

ApeState Ape::getState() const { return currentState; }

void Ape::setState(ApeState state) {
    if (currentState == ApeState::Grounded && state == ApeState::Airborne && !droppingThrough) {
        // We fell off a ledge, start coyote time
        if (velocity.y >= 0.f) coyoteTimer = coyoteTimeMax;
    }
    
    currentState = state;
    if (state == ApeState::Airborne) droppingThrough = false;
    if (state == ApeState::Grounded) coyoteTimer = 0.f;
}

bool Ape::isDroppingThrough() const { return droppingThrough; }
void Ape::setDroppingThrough(bool drop) { droppingThrough = drop; }
void Ape::resetCoyoteTime() { coyoteTimer = coyoteTimeMax; }

void Ape::updateVisuals() {
    switch (currentState) {
        case ApeState::Grounded: setColor(sf::Color(200, 50, 50)); break;
        case ApeState::Airborne: setColor(sf::Color(250, 100, 100)); break;
        case ApeState::ClimbingTrunk: setColor(sf::Color(50, 200, 50)); break;
        case ApeState::ClimbingVine: setColor(sf::Color(50, 250, 150)); break;
        case ApeState::HangingBranch: setColor(sf::Color(200, 200, 50)); break;
    }
}