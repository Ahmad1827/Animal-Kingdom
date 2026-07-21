#include "graphics/Animator.h"

Animator::Animator(sf::Sprite* sprite) 
    : targetSprite(sprite), currentAnimation(nullptr), currentTime(0.f), currentFrame(0), facingRight(true), paused(false) {}

void Animator::addAnimation(const std::string& name, int startX, int startY, int frameWidth, int frameHeight, int frameCount, float fps, bool loop, float offsetX, float offsetY) {
    Animation anim;
    anim.name = name;
    anim.startX = startX;
    anim.startY = startY;
    anim.frameWidth = frameWidth;
    anim.frameHeight = frameHeight;
    anim.frameCount = frameCount;
    anim.frameDuration = 1.0f / fps;
    anim.looping = loop;
    anim.offsetX = offsetX;
    anim.offsetY = offsetY;
    
    animations[name] = anim;
}

void Animator::play(const std::string& name) {
    if (currentAnimation && currentAnimation->name == name) return;
    
    if (animations.find(name) != animations.end()) {
        currentAnimation = &animations[name];
        currentTime = 0.f;
        currentFrame = 0;
        updateSpriteRect();
    }
}

void Animator::pause() { paused = true; }
void Animator::resume() { paused = false; }

void Animator::update(float dt) {
    if (!currentAnimation || paused) return;

    currentTime += dt;
    // Fix Bug 1: Safe while-loop prevents frame timer corruption
    while (currentTime >= currentAnimation->frameDuration) {
        currentTime -= currentAnimation->frameDuration;
        currentFrame++;

        if (currentFrame >= currentAnimation->frameCount) {
            if (currentAnimation->looping) {
                currentFrame = 0;
            } else {
                currentFrame = currentAnimation->frameCount - 1;
                currentTime = 0.f; // Prevent infinite time accumulation
                break;
            }
        }
    }
    updateSpriteRect();
}

void Animator::setFacingRight(bool right) {
    if (facingRight != right) {
        facingRight = right;
        updateSpriteRect();
    }
}

void Animator::updateSpriteRect() {
    if (!currentAnimation || !targetSprite) return;

    int x = (currentAnimation->startX + currentFrame) * currentAnimation->frameWidth;
    int y = currentAnimation->startY * currentAnimation->frameHeight;
    int w = currentAnimation->frameWidth;
    int h = currentAnimation->frameHeight;

    if (facingRight) {
        targetSprite->setTextureRect(sf::IntRect(x, y, w, h));
    } else {
        targetSprite->setTextureRect(sf::IntRect(x + w, y, -w, h));
    }
}

bool Animator::isFacingRight() const { return facingRight; }
std::string Animator::getCurrentAnimationName() const { return currentAnimation ? currentAnimation->name : "None"; }
int Animator::getCurrentFrame() const { return currentFrame; }
float Animator::getCurrentTime() const { return currentTime; }
float Animator::getFPS() const { return currentAnimation ? (1.0f / currentAnimation->frameDuration) : 0.f; }
sf::Vector2f Animator::getCurrentOffset() const { return currentAnimation ? sf::Vector2f(currentAnimation->offsetX, currentAnimation->offsetY) : sf::Vector2f(0.f, 0.f); }
sf::IntRect Animator::getCurrentRect() const { return targetSprite ? targetSprite->getTextureRect() : sf::IntRect(); }