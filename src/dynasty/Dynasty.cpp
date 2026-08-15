// src/dynasty/Dynasty.cpp
#include "dynasty/Dynasty.h"
#include <algorithm>

void Dynasty::registerMember(Character::ID charId) {
    if (std::find(memberIds.begin(), memberIds.end(), charId) == memberIds.end()) {
        memberIds.push_back(charId);
    }
}

void Dynasty::setAlpha(Character::ID newAlphaId) {
    if (currentAlphaId != Character::INVALID_ID && currentAlphaId != newAlphaId) {
        historicalAlphaIds.push_back(currentAlphaId);
    }
    currentAlphaId = newAlphaId;
}