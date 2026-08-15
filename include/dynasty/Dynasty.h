// include/dynasty/Dynasty.h
#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include "dynasty/Character.h"

class Dynasty {
public:
    uint64_t id = 0;
    std::string name;
    Character::ID founderId = Character::INVALID_ID;
    Character::ID currentAlphaId = Character::INVALID_ID;
    int prestige = 0;

    std::vector<Character::ID> memberIds;
    std::vector<Character::ID> historicalAlphaIds;

    void registerMember(Character::ID charId);
    void setAlpha(Character::ID newAlphaId);
};