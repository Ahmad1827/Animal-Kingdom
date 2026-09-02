#include "entities/Ape.h"
#include <cmath>
#include <iostream>
#include <memory>

sf::Texture Ape::masterSpriteSheet;
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
      depthLane(isPlayer ? sim::DepthLane::Foreground : sim::DepthLane::Midground),
      currentAnimState(AnimState::Idle),
      animTimer(0.f),
      currentFrame(0),
      facingRight(true) {
    
    if (!newTexturesLoaded) {
        masterSpriteSheet.loadFromFile("assets/sprites/spritesheet.png");
        newTexturesLoaded = true;
    }

    bounds = sf::FloatRect(x, y, 50.f, 50.f);
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
    animator.addAnimation("Work",  0, 0, frameW, frameH, 4,  8.f,  true,  0.f, 0.f); 

    float visualScale = 0.58f; 
    sprite.setScale(visualScale, visualScale);
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

void Ape::setDepthLane(sim::DepthLane lane) {
    depthLane = lane;
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

    AnimState nextState = AnimState::Idle;

    if (state == ApeState::Grounded || state == ApeState::Working) {
        if (isMovingHorizontally) nextState = AnimState::Walk;
        else nextState = AnimState::Idle;
    } 
    else if (state == ApeState::Airborne || state == ApeState::HangingBranch) {
        nextState = AnimState::Jump;
    } 
    else if (state == ApeState::ClimbingTrunk || state == ApeState::ClimbingVine) {
        nextState = AnimState::Climb;
    }

    if (nextState != currentAnimState) {
        currentAnimState = nextState;
        currentFrame = 0;
        animTimer = 0.f;
    }

    float laneScaleMultiplier = 1.0f;
    float laneYOffset = 0.f;
    sf::Color laneColor = sf::Color::White;

    if (depthLane == sim::DepthLane::Background) {
        laneScaleMultiplier = 0.72f;
        laneYOffset = -52.f;
        laneColor = sf::Color(185, 190, 210);
    } else if (depthLane == sim::DepthLane::Midground) {
        laneScaleMultiplier = 0.95f;
        laneYOffset = 0.f;
        laneColor = sf::Color(240, 240, 245);
    } else {
        laneScaleMultiplier = 1.15f;
        laneYOffset = 6.f;
        laneColor = sf::Color(255, 255, 255);
    }

    float baseScale = 0.58f * laneScaleMultiplier;
    sf::Vector2f renderOffset(0.f, laneYOffset);

    animTimer += dt;
    int stateIdx = static_cast<int>(currentAnimState);
    int maxFrames = FRAMES_PER_STATE[stateIdx];
    float frameDur = DURATION_PER_STATE[stateIdx];
    
    bool isClimbingAndStill = (currentAnimState == AnimState::Climb && std::abs(velocity.y) < 5.f && std::abs(velocity.x) < 5.f);
    
    if (animTimer >= frameDur && !isClimbingAndStill) {
        animTimer -= frameDur;
        currentFrame++;
        
        if (currentAnimState == AnimState::Jump && currentFrame >= maxFrames) {
            currentFrame = maxFrames - 1;
        } else {
            currentFrame %= maxFrames;
        }
    }
    
    sprite.setTexture(masterSpriteSheet);
    
    int sheetColumns = 8;
    int sheetRows = 4;
    int fw = sprite.getTexture()->getSize().x / sheetColumns;
    int fh = sprite.getTexture()->getSize().y / sheetRows;
    
    sprite.setTextureRect(sf::IntRect(currentFrame * fw, stateIdx * fh, fw, fh));
    sprite.setOrigin(fw / 2.f, static_cast<float>(fh));
    sprite.setColor(laneColor);
    
    float flipScale = facingRight ? 1.f : -1.f;
    sprite.setScale(baseScale * flipScale * landingDetector.squashScaleX, baseScale * landingDetector.squashScaleY);

    landingDetector.updateSquash(dt);
    sprite.setPosition(bounds.left + bounds.width / 2.f + renderOffset.x, bounds.top + bounds.height + renderOffset.y);
}

void Ape::draw(sf::RenderTarget& target) {
    target.draw(sprite);

    sf::Vector2f center = sprite.getPosition();
    center.y -= sprite.getGlobalBounds().height / 2.f;
    
    float facingDir = facingRight ? 1.f : -1.f;
    float laneScaleMultiplier = (depthLane == sim::DepthLane::Background) ? 0.72f : (depthLane == sim::DepthLane::Midground ? 0.95f : 1.15f);

    if (isKing) {
        sf::ConvexShape crown(3);
        crown.setPoint(0, sf::Vector2f(-14.f * laneScaleMultiplier, 0.f));
        crown.setPoint(1, sf::Vector2f(14.f * laneScaleMultiplier, 0.f));
        crown.setPoint(2, sf::Vector2f(0.f, -22.f * laneScaleMultiplier));
        crown.setFillColor(sf::Color(255, 215, 0));
        crown.setOutlineColor(sf::Color(184, 134, 11));
        crown.setOutlineThickness(1.f);
        crown.setPosition(center.x + (26.f * facingDir * laneScaleMultiplier), center.y - (52.f * laneScaleMultiplier));
        target.draw(crown);
    }

    if (currentTool == sim::ToolType::StoneAxe) {
        sf::RectangleShape handle(sf::Vector2f(6.f * laneScaleMultiplier, 36.f * laneScaleMultiplier));
        handle.setFillColor(sf::Color(139, 69, 19));
        handle.setOrigin(3.f * laneScaleMultiplier, 18.f * laneScaleMultiplier);
        handle.setPosition(center.x + (24.f * facingDir * laneScaleMultiplier), center.y);
        target.draw(handle);
        
        sf::RectangleShape head(sf::Vector2f(18.f * laneScaleMultiplier, 14.f * laneScaleMultiplier));
        head.setFillColor(sf::Color(105, 105, 105));
        head.setOrigin(9.f * laneScaleMultiplier, 7.f * laneScaleMultiplier);
        head.setPosition(center.x + (28.f * facingDir * laneScaleMultiplier), center.y - (12.f * laneScaleMultiplier));
        target.draw(head);
    } else if (currentTool == sim::ToolType::WoodenSpear) {
        sf::RectangleShape spear(sf::Vector2f(5.f * laneScaleMultiplier, 66.f * laneScaleMultiplier));
        spear.setFillColor(sf::Color(160, 82, 45));
        spear.setOrigin(2.5f * laneScaleMultiplier, 33.f * laneScaleMultiplier);
        spear.setPosition(center.x + (24.f * facingDir * laneScaleMultiplier), center.y - (8.f * laneScaleMultiplier));
        target.draw(spear);
    } else if (currentTool == sim::ToolType::Basket) {
        sf::RectangleShape basket(sf::Vector2f(28.f * laneScaleMultiplier, 22.f * laneScaleMultiplier));
        basket.setFillColor(sf::Color(218, 165, 32));
        basket.setOrigin(14.f * laneScaleMultiplier, 11.f * laneScaleMultiplier);
        basket.setPosition(center.x - (30.f * facingDir * laneScaleMultiplier), center.y + (8.f * laneScaleMultiplier));
        target.draw(basket);
    }

    if (carriedItemType == 1 || (resourceAmount > 0 && currentResource == sim::ResourceType::Food)) {
        float bx = center.x + (30.f * facingDir * laneScaleMultiplier);
        float by = center.y + (8.f * laneScaleMultiplier);
        
        sf::CircleShape banana1(8.f * laneScaleMultiplier, 3);
        banana1.setScale(0.5f, 1.5f);
        banana1.setFillColor(sf::Color(255, 225, 0));
        banana1.setPosition(bx - (7.f * laneScaleMultiplier), by);
        banana1.setRotation(15.f);
        target.draw(banana1);

        sf::CircleShape banana2(8.f * laneScaleMultiplier, 3);
        banana2.setScale(0.5f, 1.5f);
        banana2.setFillColor(sf::Color(255, 215, 0));
        banana2.setPosition(bx, by - (4.f * laneScaleMultiplier));
        target.draw(banana2);
        
        sf::CircleShape banana3(8.f * laneScaleMultiplier, 3);
        banana3.setScale(0.5f, 1.5f);
        banana3.setFillColor(sf::Color(255, 235, 0));
        banana3.setPosition(bx + (7.f * laneScaleMultiplier), by);
        banana3.setRotation(-15.f);
        target.draw(banana3);
    } 
    else if (carriedItemType == 2 || (resourceAmount > 0 && currentResource == sim::ResourceType::Wood)) {
        sf::RectangleShape log(sf::Vector2f(44.f * laneScaleMultiplier, 12.f * laneScaleMultiplier));
        log.setFillColor(sf::Color(101, 67, 33));
        log.setOutlineColor(sf::Color(60, 30, 10));
        log.setOutlineThickness(1.f);
        log.setOrigin(22.f * laneScaleMultiplier, 6.f * laneScaleMultiplier);
        log.setPosition(center.x + (8.f * facingDir * laneScaleMultiplier), center.y - (38.f * laneScaleMultiplier));
        log.setRotation(facingDir > 0 ? 15.f : -15.f);
        target.draw(log);
    } 
    else if (carriedItemType == 3 || (resourceAmount > 0 && currentResource == sim::ResourceType::Stone)) {
        sf::ConvexShape rock(5);
        rock.setPoint(0, sf::Vector2f(0.f, -12.f * laneScaleMultiplier));
        rock.setPoint(1, sf::Vector2f(10.f * laneScaleMultiplier, -6.f * laneScaleMultiplier));
        rock.setPoint(2, sf::Vector2f(12.f * laneScaleMultiplier, 6.f * laneScaleMultiplier));
        rock.setPoint(3, sf::Vector2f(0.f, 12.f * laneScaleMultiplier));
        rock.setPoint(4, sf::Vector2f(-12.f * laneScaleMultiplier, 4.f * laneScaleMultiplier));
        rock.setFillColor(sf::Color(128, 128, 128));
        rock.setOutlineColor(sf::Color(80, 80, 80));
        rock.setOutlineThickness(1.f);
        rock.setPosition(center.x + (30.f * facingDir * laneScaleMultiplier), center.y + (8.f * laneScaleMultiplier));
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