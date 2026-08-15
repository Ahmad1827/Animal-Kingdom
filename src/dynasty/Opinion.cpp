#include "dynasty/Opinion.h"
#include <algorithm>

namespace sim {

int OpinionMatrix::calculateTotal() const {
    int sum = 0;
    for (const auto& mod : modifiers) {
        sum += mod.value;
    }
    return std::clamp(sum, -100, 100);
}

void OpinionMatrix::addModifier(const std::string& reason, int value, float duration) {
    for (auto& mod : modifiers) {
        if (mod.reason == reason) {
            mod.value = value;
            mod.duration = duration;
            return;
        }
    }
    modifiers.push_back({reason, value, duration});
}

void OpinionMatrix::removeModifier(const std::string& reason) {
    modifiers.erase(
        std::remove_if(modifiers.begin(), modifiers.end(), [&](const OpinionModifier& m) {
            return m.reason == reason;
        }),
        modifiers.end()
    );
}

void OpinionMatrix::update(float dt) {
    for (auto it = modifiers.begin(); it != modifiers.end();) {
        if (it->duration > 0.0f) {
            it->duration -= dt;
            if (it->duration <= 0.0f) {
                it = modifiers.erase(it);
                continue;
            }
        }
        ++it;
    }
}

}