#include "entities/Ape.h"
#include <cmath>
#include <iostream>

Ape::Ape(float x, float y, sf::Texture& texture) {
    bounds = sf::FloatRect(x, y, 32.f, 32.f); 
    velocity = sf::Vector2f(0.f, 0.f);
    state = ApeState::Airborne;
    droppingThrough = false;

    int texW = texture.getSize().x;
    int texH = texture.getSize().y;
    int frameW = texW > 0 ? texW / 8 : 240;
    int frameH = texH > 0 ? texH / 6 : 174;

    if (texW > 0) {
        sprite.setTexture(texture);
        sprite.setOrigin(frameW / 2.f, frameH); 
        
        // BUG 1 FIX: Uncouple the rendering scale from the 32x32 bounds.
        // The ape occupies only a portion of the 174px frame height. 
        // 0.35f scales the visible character to roughly match the physical bounds.
        float visualScale = 0.35f; 
        sprite.setScale(visualScale, visualScale);
    }

    animator = std::make_unique<Animator>(&sprite);
    
    // BUG 2 FIX: Added negative Y offsets to prevent feet from sliding/jittering.
    // Hanging animations require a massive negative Y offset because the hands 
    // are at the top of the frame, but the physics anchor is at the bottom.
    animator->addAnimation("Idle",  0, 0, frameW, frameH, 8, 6.f,  true,   0.f, 0.f);
    animator->addAnimation("Walk",  0, 1, frameW, frameH, 8, 10.f, true,   0.f, 2.f);
    animator->addAnimation("Run",   0, 2, frameW, frameH, 8, 14.f, true,   0.f, 4.f);
    animator->addAnimation("Jump",  0, 3, frameW, frameH, 2, 6.f,  false,  0.f, 8.f);
    animator->addAnimation("Fall",  2, 3, frameW, frameH, 2, 6.f,  true,   0.f, 8.f);
    animator->addAnimation("Land",  4, 3, frameW, frameH, 2, 6.f,  false,  0.f, 0.f);
    animator->addAnimation("Climb", 0, 4, frameW, frameH, 8, 10.f, true,   0.f, 0.f);
    animator->addAnimation("Hang",  0, 5, frameW, frameH, 6, 4.f,  true,   0.f, -24.f);
    animator->addAnimation("Swing", 0, 5, frameW, frameH, 6, 12.f, true,   0.f, -24.f);

    animator->play("Idle");
}

void Ape::update(float dt) {
    float moveSpeed = 200.f;
    
    // --- GROUND / AIR PHYSICS ---
    if (state == ApeState::Grounded || state == ApeState::Airborne) {
        velocity.x = 0.f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            velocity.x = -moveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            velocity.x = moveSpeed;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
            velocity.x *= 1.5f;
        }
    }

    if (state == ApeState::Grounded) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            velocity.y = -600.f;
            state = ApeState::Airborne;
        }
    }

    if (state == ApeState::Airborne) {
        velocity.y += 1000.f * dt;
    }

    // --- BUG 3 FIX: RESTORED CLIMBING & HANGING PHYSICS ---
    if (state == ApeState::ClimbingTrunk || state == ApeState::ClimbingVine) {
        velocity.x = 0.f;
        velocity.y = 0.f; // Defy gravity while holding on
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            velocity.y = -150.f;
        } 
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            velocity.y = 150.f;
        }

        // Exit climb by jumping
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            velocity.y = -400.f;
            state = ApeState::Airborne;
        }
    }

    if (state == ApeState::HangingBranch) {
        velocity.y = 0.f; 
        velocity.x = 0.f;
        
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            velocity.x = -150.f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            velocity.x = 150.f;
        }

        // Drop down
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            state = ApeState::Airborne;
        }
    }

    // Apply movement
    bounds.left += velocity.x * dt;
    bounds.top += velocity.y * dt;

    // --- ANIMATION MAPPING ---
    if (velocity.x > 5.f) animator->setFacingRight(true);
    else if (velocity.x < -5.f) animator->setFacingRight(false);

    animator->resume();

    if (state == ApeState::Airborne) {
        if (velocity.y < -20.f) animator->play("Jump");
        else animator->play("Fall");
    } 
    else if (state == ApeState::Grounded) {
        if (std::abs(velocity.x) > 150.f) animator->play("Run");
        else if (std::abs(velocity.x) > 10.f) animator->play("Walk");
        else animator->play("Idle");
    } 
    else if (state == ApeState::ClimbingTrunk || state == ApeState::ClimbingVine) {
        animator->play("Climb");
        if (std::abs(velocity.y) < 5.f && std::abs(velocity.x) < 5.f) {
            animator->pause();
        }
    } 
    else if (state == ApeState::HangingBranch) {
        if (std::abs(velocity.x) > 10.f) animator->play("Swing");
        else animator->play("Hang");
    }

    animator->update(dt);

    sf::Vector2f renderOffset = animator->getCurrentOffset();
    if (!animator->isFacingRight()) {
        renderOffset.x = -renderOffset.x; 
    }

    // Sprite position strictly anchors to the unpolluted physics collision box
    sprite.setPosition(bounds.left + bounds.width / 2.f + renderOffset.x, bounds.top + bounds.height + renderOffset.y);
}

void Ape::draw(sf::RenderWindow& window) const {
    window.draw(sprite);
}

sf::FloatRect Ape::getBounds() const { return bounds; }
sf::Vector2f Ape::getPosition() const { return sf::Vector2f(bounds.left, bounds.top); }
sf::Vector2f Ape::getVelocity() const { return velocity; }
ApeState Ape::getState() const { return state; }
bool Ape::isDroppingThrough() const { return droppingThrough; }

void Ape::setPosition(float x, float y) { bounds.left = x; bounds.top = y; }
void Ape::setVelocity(float vx, float vy) { velocity.x = vx; velocity.y = vy; }
void Ape::setState(ApeState newState) { state = newState; }
void Ape::setDroppingThrough(bool drop) { droppingThrough = drop; }
Animator* Ape::getAnimator() const { return animator.get(); }