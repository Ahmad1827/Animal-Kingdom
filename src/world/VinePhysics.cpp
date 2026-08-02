#include "world/VinePhysics.h"
#include <cmath>

VinePhysics::VinePhysics(const sf::Vector2f& startPos, int numSegments, float length, float damp) 
    : segmentLength(length), damping(damp), anchor(startPos) {
    for (int i = 0; i < numSegments; ++i) {
        VineSegment seg;
        seg.position = startPos + sf::Vector2f(0.f, i * length);
        seg.oldPosition = seg.position;
        segments.push_back(seg);
    }
}

void VinePhysics::update(float dt) {
    sf::Vector2f gravity(0.f, 980.f);
    
    // Verlet Integration step
    for (size_t i = 1; i < segments.size(); ++i) {
        sf::Vector2f velocity = (segments[i].position - segments[i].oldPosition) * damping;
        segments[i].oldPosition = segments[i].position;
        segments[i].position += velocity + gravity * dt * dt;
    }
    
    segments[0].position = anchor;
    segments[0].oldPosition = anchor;

    // Constraint resolution (keeps segments attached properly)
    for (int iterations = 0; iterations < 5; ++iterations) {
        for (size_t i = 0; i < segments.size() - 1; ++i) {
            sf::Vector2f delta = segments[i + 1].position - segments[i].position;
            float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            float diff = (segmentLength - dist) / dist;
            sf::Vector2f offset = delta * 0.5f * diff;
            
            if (i != 0) {
                segments[i].position -= offset;
            }
            segments[i + 1].position += offset;
        }
    }
}

void VinePhysics::applyForce(int segmentIndex, const sf::Vector2f& force) {
    if (segmentIndex > 0 && segmentIndex < segments.size()) {
        segments[segmentIndex].oldPosition -= force;
    }
}

void VinePhysics::setSegmentPosition(int segmentIndex, const sf::Vector2f& pos) {
    if (segmentIndex >= 0 && segmentIndex < segments.size()) {
        segments[segmentIndex].position = pos;
    }
}

sf::Vector2f VinePhysics::getSegmentVelocity(int segmentIndex, float dt) const {
    if (segmentIndex >= 0 && segmentIndex < segments.size() && dt > 0.f) {
        return (segments[segmentIndex].position - segments[segmentIndex].oldPosition) / dt;
    }
    return sf::Vector2f(0.f, 0.f);
}

sf::Vector2f VinePhysics::getSegmentPosition(int segmentIndex) const {
    if (segmentIndex >= 0 && segmentIndex < segments.size()) {
        return segments[segmentIndex].position;
    }
    return sf::Vector2f(0.f, 0.f);
}

int VinePhysics::getSegmentCount() const { return segments.size(); }    