#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace sim {

struct OpinionModifier {
    std::string reason;
    int value;
    float duration = -1.0f;
};

class OpinionMatrix {
public:
    std::vector<OpinionModifier> modifiers;

    int calculateTotal() const;
    void addModifier(const std::string& reason, int value, float duration = -1.0f);
    void removeModifier(const std::string& reason);
    void update(float dt);
};

}