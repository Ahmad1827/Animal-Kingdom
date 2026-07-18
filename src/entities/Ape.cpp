#include "entities/Ape.h"
#include <algorithm>

Ape::Ape(float x, float y) : Entity(x, y, 28.f, 28.f, sf::Color::Red) {
    speed = 250.f;
    climbSpeed = 200.f;
    gravity = 1200.f;
    jumpForce = -550.f;
    currentState = ApeState::Airborne;
    droppingThrough = false;
}

void Ape::processInput() {
    if (currentState == ApeState::ClimbingTrunk) {
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
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            setState(ApeState::Airborne);
            velocity.y = jumpForce;
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
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            setState(ApeState::Airborne);
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
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                    droppingThrough = true;
                    setState(ApeState::Airborne);
                }
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                velocity.y = jumpForce;
                setState(ApeState::Airborne);
            }
        }
    }
}

void Ape::update(float dt) {
    processInput();
    
    if (currentState == ApeState::Airborne || currentState == ApeState::Grounded) {
        velocity.y += gravity * dt;
    }

    position += velocity * dt;
    shape.setPosition(position);
    updateVisuals();
}

ApeState Ape::getState() const {
    return currentState;
}

void Ape::setState(ApeState state) {
    currentState = state;
    if (state == ApeState::Airborne) {
        droppingThrough = false;
    }
}

bool Ape::isDroppingThrough() const {
    return droppingThrough;
}

void Ape::setDroppingThrough(bool drop) {
    droppingThrough = drop;
}

void Ape::updateVisuals() {
    switch (currentState) {
        case ApeState::Grounded: setColor(sf::Color(200, 50, 50)); break;
        case ApeState::Airborne: setColor(sf::Color(250, 100, 100)); break;
        case ApeState::ClimbingTrunk: setColor(sf::Color(50, 200, 50)); break;
        case ApeState::HangingBranch: setColor(sf::Color(200, 200, 50)); break;
    }
}