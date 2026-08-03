#pragma once
#include <cstdint>

namespace sim {

class WorldClock {
private:
    float accumulator;
    float tickInterval;
    uint64_t totalTicks;

    int seconds;
    int minutes;
    int hours;
    int days;
    int seasons;
    int years;

public:
    WorldClock();

    void update(float dt);
    bool consumeTick();

    uint64_t getTotalTicks() const;
    int getSeconds() const;
    int getMinutes() const;
    int getHours() const;
    int getDays() const;
    int getSeasons() const;
    int getYears() const;
};

}