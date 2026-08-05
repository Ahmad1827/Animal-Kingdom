#include "entities/Ape.h"
#include <cmath>
#include <iostream>
#include <memory>

Ape::Ape(float x, float y, sf::Texture& texture, bool isPlayer) 
    : texture(texture), 
      animator(&sprite),
      state(ApeState::Airborne), 
      droppingThrough(false), 
      isPlayer(isPlayer), 
      carriedItemType(0),
      currentTool(sim::ToolType::None), 
      currentResource(sim::ResourceType::None), 
      resourceAmount(0), 
      isKing(false) {
    
    bounds = sf::FloatRect(x, y, 32.f, 32.f);
    velocity = sf::Vector2f(0.f, 0.f);

    int texW = texture.getSize().x;
    int texH = texture.getSize().y;

    int columns = 11;
    int rows = 6;
    
    int frameW = texW > 0 ? texW / columns : 240;
    int frameH = texH > 0 ? texH / rows : 174;

    if (texW > 0) {
        sprite.setTexture(texture);
        sprite.setOrigin(frameW / 2.f, static_cast<float>(frameH)); 
        
        float visualScale = 1.20f; 
        sprite.setScale(visualScale, visualScale);
    }
    
    animator.addAnimation("Idle",  0, 0, frameW, frameH, 8,  6.f,  true,  0.f, 0.f);
    animator.addAnimation("Walk",  0, 1, frameW, frameH, 8,  10.f, true,  0.f, 0.f);
    animator.addAnimation("Run",   0, 2, frameW, frameH, 11, 14.f, true,  0.f, 0.f);
    animator.addAnimation("Jump",  0, 3, frameW, frameH, 2,  6.f,  false, 0.f, 0.f);
    animator.addAnimation("Fall",  2, 3, frameW, frameH, 2,  6.f,  true,  0.f, 0.f);
    animator.addAnimation("Land",  4, 3, frameW, frameH, 2,  6.f,  false, 0.f, 0.f);
    animator.addAnimation("Climb", 0, 4, frameW, frameH, 10, 10.f, true,  0.f, 0.f);
    animator.addAnimation("Hang",  0, 5, frameW, frameH, 5,  4.f,  true,  0.f, 0.f);
    animator.addAnimation("Swing", 5, 5, frameW, frameH, 4,  12.f, true,  0.f, 0.f);

    animator.play("Idle");
}

void Ape::setCarriedItem(int itemType) {
    carriedItemType = itemType;
}

void Ape::setVisualEquipment(sim::ToolType tool, sim::ResourceType res, int amount, bool king) {
    currentTool = tool;
    currentResource = res;
    resourceAmount = amount;
    isKing = king;
}

ImpactLevel Ape::registerLanding(float impactVelocityY) {
    return landingDetector.registerLanding(impactVelocityY);
}

void Ape::update(float dt) {
    float moveSpeed = 200.f;
    
    if (isPlayer) {
        if (state == ApeState::Grounded || state == ApeState::Airborne) {
            if (state != ApeState::Airborne) {
                velocity.x = 0.f;
            }
            
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                if (state == ApeState::Grounded) {
                    velocity.x = -moveSpeed;
                } else {
                    velocity.x -= 800.f * dt;
                    if (velocity.x < -moveSpeed * 1.5f) velocity.x = -moveSpeed * 1.5f;
                }
            }
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                if (state == ApeState::Grounded) {
                    velocity.x = moveSpeed;
                } else {
                    velocity.x += 800.f * dt;
                    if (velocity.x > moveSpeed * 1.5f) velocity.x = moveSpeed * 1.5f;
                }
            }
            
            if (state == ApeState::Grounded && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                velocity.x *= 1.5f;
            }
        }

        if (state == ApeState::Grounded) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                velocity.y = -600.f;
                state = ApeState::Airborne;
            }
        }
    }

    if (state == ApeState::Airborne) {
        velocity.y += 1000.f * dt;
    }

    if (state == ApeState::ClimbingTrunk) {
        velocity.x = 0.f;
        velocity.y = 0.f;
        
        if (isPlayer) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
                velocity.y = -150.f;
            } 
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                velocity.y = 150.f;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                velocity.y = -400.f;
                state = ApeState::Airborne;
            }
        }
    }

    if (state == ApeState::HangingBranch) {
        velocity.y = 0.f;
        
        if (isPlayer) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                velocity.x -= 300.f * dt;
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                velocity.x += 300.f * dt;
            } else {
                velocity.x *= 0.8f; 
            }

            if (velocity.x > 100.f) velocity.x = 100.f;
            if (velocity.x < -100.f) velocity.x = -100.f;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
                velocity.y = -500.f;
                state = ApeState::Airborne;
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
                velocity.x *= 0.5f;
                state = ApeState::Airborne;
            }
        }
    }

    bounds.left += velocity.x * dt;
    bounds.top += velocity.y * dt;

    if (state == ApeState::ClimbingVine) {
        if (isPlayer) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                animator.setFacingRight(false);
            } else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                animator.setFacingRight(true);
            }
        } else {
            if (velocity.x > 5.f) animator.setFacingRight(true);
            else if (velocity.x < -5.f) animator.setFacingRight(false);
        }
    } else {
        if (velocity.x > 5.f) animator.setFacingRight(true);
        else if (velocity.x < -5.f) animator.setFacingRight(false);
    }

    animator.resume();

    bool isMovingHorizontally = (std::abs(velocity.x) > 10.f);
    bool isRunning = (std::abs(velocity.x) >= moveSpeed * 1.2f); 

    if (state == ApeState::Grounded) {
        if (isRunning) {
            animator.play("Run");
        } else if (isMovingHorizontally) {
            animator.play("Walk");
        } else {
            animator.play("Idle");
        }
    } 
    else if (state == ApeState::Airborne) {
        if (velocity.y < -150.f) {
            animator.play("Jump");
        } else if (velocity.y > 150.f) {
            animator.play("Fall");
        } else {
            if (isRunning) animator.play("Run");
            else if (isMovingHorizontally) animator.play("Walk");
            else animator.play("Idle");
        }
    } 
    else if (state == ApeState::ClimbingTrunk) {
        animator.play("Climb");
        if (std::abs(velocity.y) < 5.f) {
            animator.pause();
        }
    }
    else if (state == ApeState::ClimbingVine) {
        if (std::abs(velocity.x) > 50.f) {
            animator.play("Swing");
        } else {
            animator.play("Climb");
            if (std::abs(velocity.y) < 5.f && std::abs(velocity.x) < 5.f) {
                animator.pause();
            }
        }
    }
    else if (state == ApeState::HangingBranch) {
        animator.play("Hang");
    }

    animator.update(dt);
    landingDetector.updateSquash(dt);

    sf::Vector2f renderOffset = animator.getCurrentOffset();
    if (!animator.isFacingRight()) {
        renderOffset.x = -renderOffset.x; 
    }

    float baseScale = 1.20f;
    sprite.setScale(baseScale * landingDetector.squashScaleX, baseScale * landingDetector.squashScaleY);
    sprite.setPosition(bounds.left + bounds.width / 2.f + renderOffset.x, bounds.top + bounds.height + renderOffset.y);
}

void Ape::draw(sf::RenderTarget& target) {
    target.draw(sprite);

    sf::Vector2f center = sprite.getPosition();
    center.y -= sprite.getGlobalBounds().height / 2.f;

    if (isKing) {
        sf::ConvexShape crown(3);
        crown.setPoint(0, sf::Vector2f(-10.f, 0.f));
        crown.setPoint(1, sf::Vector2f(10.f, 0.f));
        crown.setPoint(2, sf::Vector2f(0.f, -14.f));
        crown.setFillColor(sf::Color(255, 215, 0));
        crown.setOutlineColor(sf::Color(184, 134, 11));
        crown.setOutlineThickness(1.f);
        crown.setPosition(center.x, center.y - 25.f);
        target.draw(crown);
    }

    if (currentTool == sim::ToolType::StoneAxe) {
        sf::RectangleShape handle(sf::Vector2f(4.f, 24.f));
        handle.setFillColor(sf::Color(139, 69, 19));
        handle.setOrigin(2.f, 12.f);
        handle.setPosition(center.x + (sprite.getScale().x > 0 ? 15.f : -15.f), center.y);
        target.draw(handle);
        
        sf::RectangleShape head(sf::Vector2f(12.f, 10.f));
        head.setFillColor(sf::Color(105, 105, 105));
        head.setOrigin(6.f, 5.f);
        head.setPosition(center.x + (sprite.getScale().x > 0 ? 18.f : -18.f), center.y - 8.f);
        target.draw(head);
    } else if (currentTool == sim::ToolType::WoodenSpear) {
        sf::RectangleShape spear(sf::Vector2f(3.f, 45.f));
        spear.setFillColor(sf::Color(160, 82, 45));
        spear.setOrigin(1.5f, 22.5f);
        spear.setPosition(center.x + (sprite.getScale().x > 0 ? 15.f : -15.f), center.y - 5.f);
        target.draw(spear);
    } else if (currentTool == sim::ToolType::Basket) {
        sf::RectangleShape basket(sf::Vector2f(18.f, 14.f));
        basket.setFillColor(sf::Color(218, 165, 32));
        basket.setOrigin(9.f, 7.f);
        basket.setPosition(center.x - (sprite.getScale().x > 0 ? 20.f : -20.f), center.y + 5.f);
        target.draw(basket);
    }

    if (resourceAmount > 0) {
        sf::CircleShape resShape(8.f);
        if (currentResource == sim::ResourceType::Food) resShape.setFillColor(sf::Color(220, 20, 60));
        else if (currentResource == sim::ResourceType::Wood) resShape.setFillColor(sf::Color(139, 69, 19));
        else if (currentResource == sim::ResourceType::Stone) resShape.setFillColor(sf::Color(169, 169, 169));
        
        resShape.setOrigin(8.f, 8.f);
        resShape.setPosition(center.x, center.y - 15.f);
        target.draw(resShape);
    }
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
void Ape::setIsPlayer(bool player) { isPlayer = player; }
Animator* Ape::getAnimator() { return &animator; }
sf::Sprite& Ape::getSprite() { return sprite; }