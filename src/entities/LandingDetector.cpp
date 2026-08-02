#include "entities/LandingDetector.h"
#include <algorithm>
#include <iostream>

ImpactLevel LandingDetector::registerLanding(float impactVelocityY) {
    std::cout << "[IMPACT] Hit the ground with velocity: " << impactVelocityY << std::endl;
    
    // EXTREMELY LOW THRESHOLDS FOR TESTING
    if (impactVelocityY > 200.f) { 
        std::cout << "[IMPACT] Level: HEAVY!" << std::endl;
        squashTimer = 0.25f;       // Lasts much longer so you can see it
        squashDuration = 0.25f;
        targetScaleX = 1.8f;       // Super wide
        targetScaleY = 0.4f;       // Super flat
        return ImpactLevel::Heavy;
    } else if (impactVelocityY > 100.f) {
        std::cout << "[IMPACT] Level: MEDIUM" << std::endl;
        squashTimer = 0.15f;
        squashDuration = 0.15f;
        targetScaleX = 1.4f;
        targetScaleY = 0.7f;
        return ImpactLevel::Medium;
    } else if (impactVelocityY > 50.f) {
        std::cout << "[IMPACT] Level: LIGHT" << std::endl;
        squashTimer = 0.10f;
        squashDuration = 0.10f;
        targetScaleX = 1.15f;
        targetScaleY = 0.85f;
        return ImpactLevel::Light;
    }
    
    return ImpactLevel::None;
}

void LandingDetector::updateSquash(float dt) {
    if (squashTimer > 0.f) {
        squashTimer -= dt;
        float t = std::max(squashTimer / squashDuration, 0.0f);
        
        // Interpolate between normal scale (1.0) and the target impact scale
        squashScaleX = 1.0f + (targetScaleX - 1.0f) * t;
        squashScaleY = 1.0f + (targetScaleY - 1.0f) * t;
    } else {
        squashScaleX = 1.0f;
        squashScaleY = 1.0f;
    }
}