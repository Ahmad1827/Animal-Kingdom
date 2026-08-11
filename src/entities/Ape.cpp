#include "entities/Ape.h"
#include <cmath>
#include <iostream>
#include <memory>

sf::Texture Ape::texIdle;
sf::Texture Ape::texWalkRight;
sf::Texture Ape::texJump;
bool Ape::newTexturesLoaded = false;

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
      isKing(false),
      currentAnimState(AnimState::OldSystem),
      animTimer(0.f),
      currentFrame(0),
      facingRight(true) {
    
    if (!newTexturesLoaded) {
        texIdle.loadFromFile("assets/sprites/idle.png");
        texWalkRight.loadFromFile("assets/sprites/walkright.png");
        texJump.loadFromFile("assets/sprites/jump.png");
        newTexturesLoaded = true;
    }

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
    
    // Fallback Work animation (reusing Idle row) so it doesn't break your sprite sheet.
    animator.addAnimation("Work",  0, 0, frameW, frameH, 4,  8.f,  true,  0.f, 0.f); 

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

    // Track intended facing direction natively for crisp visual flips
    if (state == ApeState::ClimbingVine) {
        if (isPlayer) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) facingRight = false;
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) facingRight = true;
        } else {
            if (velocity.x > 5.f) facingRight = true;
            else if (velocity.x < -5.f) facingRight = false;
        }
    } else {
        if (velocity.x > 5.f) facingRight = true;
        else if (velocity.x < -5.f) facingRight = false;
    }

    bool isMovingHorizontally = (std::abs(velocity.x) > 10.f);
    bool isRunning = (std::abs(velocity.x) >= moveSpeed * 1.2f); 

    AnimState nextState = AnimState::OldSystem;
    std::string nextOldAnim = "Idle";

    // 1. Determine the appropriate State logic
    if (state == ApeState::Grounded) {
        if (isRunning) {
            nextState = AnimState::OldSystem;
            nextOldAnim = "Run";
        } else if (isMovingHorizontally) {
            nextState = AnimState::Walk;
        } else {
            nextState = AnimState::Idle;
        }
    } 
    else if (state == ApeState::Working) {
        nextState = AnimState::OldSystem;
        nextOldAnim = "Work";
    }
    else if (state == ApeState::Airborne) {
        nextState = AnimState::Jump; // Use Jump visual until Fall is added
    } 
    else if (state == ApeState::ClimbingTrunk) {
        nextState = AnimState::OldSystem;
        nextOldAnim = "Climb";
        if (std::abs(velocity.y) < 5.f) animator.pause();
        else animator.resume();
    }
    else if (state == ApeState::ClimbingVine) {
        nextState = AnimState::OldSystem;
        if (std::abs(velocity.x) > 50.f) {
            nextOldAnim = "Swing";
            animator.resume();
        } else {
            nextOldAnim = "Climb";
            if (std::abs(velocity.y) < 5.f && std::abs(velocity.x) < 5.f) animator.pause();
            else animator.resume();
        }
    }
    else if (state == ApeState::HangingBranch) {
        nextState = AnimState::OldSystem;
        nextOldAnim = "Hang";
        animator.resume();
    }

    // 2. Handle State Transitions Safely
    if (nextState != currentAnimState) {
        currentAnimState = nextState;
        currentFrame = 0;
        animTimer = 0.f;
        if (currentAnimState == AnimState::OldSystem) {
            sprite.setTexture(texture);
            int frameW = texture.getSize().x > 0 ? texture.getSize().x / 11 : 240;
            int frameH = texture.getSize().y > 0 ? texture.getSize().y / 6 : 174;
            sprite.setOrigin(frameW / 2.f, static_cast<float>(frameH));
        }
    }

    // 3. Render Setup
    float baseScale = 1.20f;
    sf::Vector2f renderOffset(0.f, 0.f);

    if (currentAnimState == AnimState::OldSystem) {
        animator.setFacingRight(facingRight);
        animator.play(nextOldAnim);
        animator.update(dt);
        
        renderOffset = animator.getCurrentOffset();
        if (!facingRight) {
            renderOffset.x = -renderOffset.x; 
        }
        
        sprite.setScale(baseScale * landingDetector.squashScaleX, baseScale * landingDetector.squashScaleY);
    } else {
        // 4. Update the New Sprite Sheet Frame Loop 
        animTimer += dt;
        const float FRAME_DURATION = 0.12f;
        int maxFrames = (currentAnimState == AnimState::Walk) ? 8 : 6;
        
        if (animTimer >= FRAME_DURATION) {
            animTimer -= FRAME_DURATION;
            currentFrame++;
            
            if (currentAnimState == AnimState::Jump && currentFrame >= maxFrames) {
                currentFrame = maxFrames - 1; // Clamp at apex for jump
            } else {
                currentFrame %= maxFrames; // Normal loop
            }
        }
        
        if (currentAnimState == AnimState::Idle) sprite.setTexture(texIdle);
        else if (currentAnimState == AnimState::Walk) sprite.setTexture(texWalkRight); // Using horizontal flip
        else if (currentAnimState == AnimState::Jump) sprite.setTexture(texJump);
        
        // 5. Calculate rect bounds and bottom-center origin for perfectly smooth transitions
        int fw = sprite.getTexture()->getSize().x / maxFrames;
        int fh = sprite.getTexture()->getSize().y;
        
        sprite.setTextureRect(sf::IntRect(currentFrame * fw, 0, fw, fh));
        sprite.setOrigin(fw / 2.f, static_cast<float>(fh));
        
        float flipScale = facingRight ? 1.f : -1.f;
        sprite.setScale(baseScale * flipScale * landingDetector.squashScaleX, baseScale * landingDetector.squashScaleY);
    }

    landingDetector.updateSquash(dt);
    // Bind position to bottom-center of the physics collision bounds
    sprite.setPosition(bounds.left + bounds.width / 2.f + renderOffset.x, bounds.top + bounds.height + renderOffset.y);
}

void Ape::draw(sf::RenderTarget& target) {
    target.draw(sprite);

    sf::Vector2f center = sprite.getPosition();
    center.y -= sprite.getGlobalBounds().height / 2.f;
    
    // Explicitly use the native facing direction to fix the visual accessories bug 
    float facingDir = facingRight ? 1.f : -1.f;

    if (isKing) {
        sf::ConvexShape crown(3);
        crown.setPoint(0, sf::Vector2f(-10.f, 0.f));
        crown.setPoint(1, sf::Vector2f(10.f, 0.f));
        crown.setPoint(2, sf::Vector2f(0.f, -14.f));
        crown.setFillColor(sf::Color(255, 215, 0));
        crown.setOutlineColor(sf::Color(184, 134, 11));
        crown.setOutlineThickness(1.f);
        crown.setPosition(center.x + (18.f * facingDir), center.y - 35.f);
        target.draw(crown);
    }

    if (currentTool == sim::ToolType::StoneAxe) {
        sf::RectangleShape handle(sf::Vector2f(4.f, 24.f));
        handle.setFillColor(sf::Color(139, 69, 19));
        handle.setOrigin(2.f, 12.f);
        handle.setPosition(center.x + (15.f * facingDir), center.y);
        target.draw(handle);
        
        sf::RectangleShape head(sf::Vector2f(12.f, 10.f));
        head.setFillColor(sf::Color(105, 105, 105));
        head.setOrigin(6.f, 5.f);
        head.setPosition(center.x + (18.f * facingDir), center.y - 8.f);
        target.draw(head);
    } else if (currentTool == sim::ToolType::WoodenSpear) {
        sf::RectangleShape spear(sf::Vector2f(3.f, 45.f));
        spear.setFillColor(sf::Color(160, 82, 45));
        spear.setOrigin(1.5f, 22.5f);
        spear.setPosition(center.x + (15.f * facingDir), center.y - 5.f);
        target.draw(spear);
    } else if (currentTool == sim::ToolType::Basket) {
        sf::RectangleShape basket(sf::Vector2f(18.f, 14.f));
        basket.setFillColor(sf::Color(218, 165, 32));
        basket.setOrigin(9.f, 7.f);
        basket.setPosition(center.x - (20.f * facingDir), center.y + 5.f);
        target.draw(basket);
    }

    if (carriedItemType == 1 || (resourceAmount > 0 && currentResource == sim::ResourceType::Food)) {
        float bx = center.x + (20.f * facingDir);
        float by = center.y + 5.f;
        
        sf::CircleShape banana1(6.f, 3);
        banana1.setScale(0.5f, 1.5f);
        banana1.setFillColor(sf::Color(255, 225, 0));
        banana1.setPosition(bx - 5.f, by);
        banana1.setRotation(15.f);
        target.draw(banana1);

        sf::CircleShape banana2(6.f, 3);
        banana2.setScale(0.5f, 1.5f);
        banana2.setFillColor(sf::Color(255, 215, 0));
        banana2.setPosition(bx, by - 2.f);
        target.draw(banana2);
        
        sf::CircleShape banana3(6.f, 3);
        banana3.setScale(0.5f, 1.5f);
        banana3.setFillColor(sf::Color(255, 235, 0));
        banana3.setPosition(bx + 5.f, by);
        banana3.setRotation(-15.f);
        target.draw(banana3);
    } 
    else if (carriedItemType == 2 || (resourceAmount > 0 && currentResource == sim::ResourceType::Wood)) {
        sf::RectangleShape log(sf::Vector2f(30.f, 8.f));
        log.setFillColor(sf::Color(101, 67, 33));
        log.setOutlineColor(sf::Color(60, 30, 10));
        log.setOutlineThickness(1.f);
        log.setOrigin(15.f, 4.f);
        log.setPosition(center.x + (5.f * facingDir), center.y - 25.f);
        log.setRotation(facingDir > 0 ? 15.f : -15.f);
        target.draw(log);
    } 
    else if (carriedItemType == 3 || (resourceAmount > 0 && currentResource == sim::ResourceType::Stone)) {
        sf::ConvexShape rock(5);
        rock.setPoint(0, sf::Vector2f(0.f, -8.f));
        rock.setPoint(1, sf::Vector2f(6.f, -4.f));
        rock.setPoint(2, sf::Vector2f(8.f, 4.f));
        rock.setPoint(3, sf::Vector2f(0.f, 8.f));
        rock.setPoint(4, sf::Vector2f(-8.f, 2.f));
        rock.setFillColor(sf::Color(128, 128, 128));
        rock.setOutlineColor(sf::Color(80, 80, 80));
        rock.setOutlineThickness(1.f);
        rock.setPosition(center.x + (20.f * facingDir), center.y + 5.f);
        target.draw(rock);
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