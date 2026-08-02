#include "entities/LandingDetector.h"
#include <algorithm>

ImpactLevel LandingDetector::registerLanding(float impactVelocityY) {
    // Balanced thresholds: 
    // < 150 is a normal hop/step. > 800 is a massive fall.
    if (impactVelocityY > 800.f) {
        squashTimer = 0.12f;
        squashDuration = 0.12f;
        targetScaleX = 1.4f;
        targetScaleY = 0.6f;
        return ImpactLevel::Heavy;
    } else if (impactVelocityY > 400.f) {
        squashTimer = 0.08f;
        squashDuration = 0.08f;
        targetScaleX = 1.2f;
        targetScaleY = 0.8f;
        return ImpactLevel::Medium;
    } else if (impactVelocityY > 150.f) {
        squashTimer = 0.06f;
        squashDuration = 0.06f;
        targetScaleX = 1.1f;
        targetScaleY = 0.9f;
        return ImpactLevel::Light;
    }
    
    return ImpactLevel::None;
}

void LandingDetector::updateSquash(float dt) {
    if (squashTimer > 0.f) {
        squashTimer -= dt;
        float t = std::max(squashTimer / squashDuration, 0.0f);
        
        squashScaleX = 1.0f + (targetScaleX - 1.0f) * t;
        squashScaleY = 1.0f + (targetScaleY - 1.0f) * t;
    } else {
        squashScaleX = 1.0f;
        squashScaleY = 1.0f;
    }
}