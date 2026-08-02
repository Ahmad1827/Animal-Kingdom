#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

struct VineSegment {
    sf::Vector2f position;
    sf::Vector2f oldPosition;
};

class VinePhysics {
private:
    std::vector<VineSegment> segments;
    float segmentLength;
    float damping;
    sf::Vector2f anchor;

public:
    VinePhysics(const sf::Vector2f& startPos, int numSegments, float length, float damp);
    void update(float dt);
    
    void applyForce(int segmentIndex, const sf::Vector2f& force);
    void setSegmentPosition(int segmentIndex, const sf::Vector2f& pos);
    
    sf::Vector2f getSegmentVelocity(int segmentIndex, float dt) const;
    sf::Vector2f getSegmentPosition(int segmentIndex) const;
    int getSegmentCount() const;
};