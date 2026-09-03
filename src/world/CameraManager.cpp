#include "world/CameraManager.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

CameraManager::CameraManager(sf::Vector2f size) {
    currentZoom = 2.85f;
    targetZoom = 2.85f;

    view.setSize(size.x * currentZoom, size.y * currentZoom);
    view.setCenter(0.f, 0.f);
    
    trauma = 0.f;
    maxShakeAngle = 4.f;
    maxShakeOffset = 30.f;
    traumaDecay = 0.8f;
    time = 0.f;
    
    tuning.deadZone = {50.f, 150.f};
    tuning.lookAheadMax = 300.f;
    tuning.lookAheadSpeed = 2.0f;
    tuning.baseLerpX = 3.5f;
    tuning.baseLerpY = 4.0f;
    tuning.fallLerpY = 8.0f;
    tuning.climbLerp = 6.0f;
    tuning.airLerpX = 2.5f;
    tuning.basePreloadMargin = 3200.f;
    tuning.baseUnloadMargin = 6400.f;

    anchorPos = {0.f, 0.f};
}

void CameraManager::addTrauma(float amount) {
    trauma = std::clamp(trauma + amount, 0.0f, 1.0f);
}

void CameraManager::update(float dt, const sf::Vector2f& targetPos, const sf::Vector2f& targetVelocity, ApeState state) {
    time += dt * 50.f;

    if (targetPos.x > anchorPos.x + tuning.deadZone.x / 2.f) {
        anchorPos.x = targetPos.x - tuning.deadZone.x / 2.f;
    } else if (targetPos.x < anchorPos.x - tuning.deadZone.x / 2.f) {
        anchorPos.x = targetPos.x + tuning.deadZone.x / 2.f;
    }

    if (targetPos.y > anchorPos.y + tuning.deadZone.y / 2.f) {
        anchorPos.y = targetPos.y - tuning.deadZone.y / 2.f;
    } else if (targetPos.y < anchorPos.y - tuning.deadZone.y / 2.f) {
        anchorPos.y = targetPos.y + tuning.deadZone.y / 2.f;
    }

    float speedRatio = std::abs(targetVelocity.x) / 300.f;
    speedRatio = std::clamp(speedRatio, 0.f, 1.f);
    
    float targetLookAheadX = 0.f;
    if (std::abs(targetVelocity.x) > 10.f) {
        targetLookAheadX = (targetVelocity.x > 0 ? 1.f : -1.f) * tuning.lookAheadMax * speedRatio;
    }
    lookAheadOffset.x += (targetLookAheadX - lookAheadOffset.x) * dt * tuning.lookAheadSpeed;

    sf::Vector2f idealPos = anchorPos;
    idealPos.x += lookAheadOffset.x;
    idealPos.y = targetPos.y + 12.f;

    float currentLerpX = tuning.baseLerpX + (speedRatio * 1.5f);
    float currentLerpY = tuning.baseLerpY;

    if (state == ApeState::Airborne) {
        if (targetVelocity.y > 200.f) {
            currentLerpY = tuning.fallLerpY;
        } else {
            currentLerpY = tuning.baseLerpY * 0.4f;
        }
        currentLerpX = tuning.airLerpX;
    } else if (state == ApeState::ClimbingTrunk || state == ApeState::ClimbingVine) {
        currentLerpX = tuning.climbLerp;
        currentLerpY = tuning.climbLerp;
    } else if (targetVelocity.x == 0.f && targetVelocity.y == 0.f) {
        currentLerpX *= 0.5f;
        currentLerpY *= 0.5f;
    }

    sf::Vector2f currentCenter = view.getCenter();
    float tX = 1.0f - std::exp(-currentLerpX * dt);
    float tY = 1.0f - std::exp(-currentLerpY * dt);

    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (idealPos.x - currentCenter.x) * tX;
    newCenter.y = currentCenter.y + (idealPos.y - currentCenter.y) * tY;

    if (std::abs(idealPos.x - newCenter.x) < 0.5f) newCenter.x = idealPos.x;
    if (std::abs(idealPos.y - newCenter.y) < 0.5f) newCenter.y = idealPos.y;

    float angle = 0.f;
    if (trauma > 0.f) {
        trauma = std::max(trauma - traumaDecay * dt, 0.0f);
        float shake = trauma * trauma; 
        shakeOffset.x = maxShakeOffset * shake * std::sin(time);
        shakeOffset.y = maxShakeOffset * shake * std::cos(time * 0.8f);
        angle = maxShakeAngle * shake * std::sin(time * 1.2f);
    } else {
        shakeOffset = {0.f, 0.f};
    }

    currentZoom += (targetZoom - currentZoom) * dt * 3.0f;
    view.setSize(1280.f * currentZoom, 720.f * currentZoom);
    
    sf::Vector2f finalCenter = newCenter + shakeOffset;
    finalCenter.x = std::floor(finalCenter.x);
    finalCenter.y = std::floor(finalCenter.y);
    
    view.setCenter(finalCenter.x, finalCenter.y);
    view.setRotation(angle); 
}

void CameraManager::updateTransition(float dt, const sf::Vector2f& targetPos) {
    float t = 1.0f - std::exp(-2.5f * dt);
    anchorPos.x += (targetPos.x - anchorPos.x) * t;
    anchorPos.y += (targetPos.y - anchorPos.y) * t;

    lookAheadOffset = {0.f, 0.f};
    trauma = 0.f;
    shakeOffset = {0.f, 0.f};

    sf::Vector2f idealPos = anchorPos;
    idealPos.y = targetPos.y + 12.f;

    sf::Vector2f currentCenter = view.getCenter();
    sf::Vector2f newCenter;
    newCenter.x = currentCenter.x + (idealPos.x - currentCenter.x) * t;
    newCenter.y = currentCenter.y + (idealPos.y - currentCenter.y) * t;

    currentZoom += (targetZoom - currentZoom) * dt * 3.0f;
    view.setSize(1280.f * currentZoom, 720.f * currentZoom);

    newCenter.x = std::floor(newCenter.x);
    newCenter.y = std::floor(newCenter.y);

    view.setCenter(newCenter.x, newCenter.y);
    view.setRotation(0.f);
}

void CameraManager::setZoom(float zoom) {
    targetZoom = zoom;
}

const sf::View& CameraManager::getView() const { return view; }
CameraTuning& CameraManager::getTuning() { return tuning; }

sf::FloatRect CameraManager::getViewBounds() const {
    sf::Vector2f center = view.getCenter();
    sf::Vector2f size = view.getSize();
    return sf::FloatRect(center.x - size.x / 2.f, center.y - size.y / 2.f, size.x, size.y);
}

sf::FloatRect CameraManager::getPreloadBounds(const sf::Vector2f& playerVelocity) const {
    sf::FloatRect bounds = getViewBounds();
    float dynamicExpandX = std::abs(playerVelocity.x) * 2.5f;
    float margin = tuning.basePreloadMargin + dynamicExpandX;
    
    bounds.left -= margin;
    bounds.top -= margin;
    bounds.width += margin * 2.f;
    bounds.height += margin * 2.f;
    return bounds;
}

sf::FloatRect CameraManager::getUnloadBounds() const {
    sf::FloatRect bounds = getViewBounds();
    float margin = tuning.baseUnloadMargin;
    bounds.left -= margin;
    bounds.top -= margin;
    bounds.width += margin * 2.f;
    bounds.height += margin * 2.f;
    return bounds;
}

sf::Vector2f CameraManager::getIdealPosition() const {
    sf::Vector2f ideal = anchorPos;
    ideal.x += lookAheadOffset.x;
    ideal.y = anchorPos.y + 12.f;
    return ideal;
}