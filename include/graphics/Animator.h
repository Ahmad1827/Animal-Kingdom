#pragma once
#include <SFML/Graphics.hpp>
#include <unordered_map>
#include <string>
#include "graphics/Animation.h"

class Animator {
private:
    std::unordered_map<std::string, Animation> animations;
    Animation* currentAnimation;
    sf::Sprite* targetSprite;
    
    float currentTime;
    int currentFrame;
    bool facingRight;
    bool paused;

    void updateSpriteRect();

public:
    Animator(sf::Sprite* sprite);

    void addAnimation(const std::string& name, int startX, int startY, int frameWidth, int frameHeight, int frameCount, float fps, bool loop, float offsetX = 0.f, float offsetY = 0.f);
    void play(const std::string& name);
    void pause();
    void resume();
    void update(float dt);
    void setFacingRight(bool right);

    bool isFacingRight() const;
    std::string getCurrentAnimationName() const;
    int getCurrentFrame() const;
    float getCurrentTime() const;
    float getFPS() const;
    sf::Vector2f getCurrentOffset() const;
    sf::IntRect getCurrentRect() const;
};