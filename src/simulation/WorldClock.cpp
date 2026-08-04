#include "simulation/WorldClock.h"

namespace sim {

WorldClock::WorldClock() 
    : accumulator(0.f), tickInterval(0.5f), totalTicks(0),
      seconds(0), minutes(0), hours(0), days(1), seasons(1), years(1) {}

void WorldClock::update(float dt) {
    accumulator += dt;
}

bool WorldClock::consumeTick() {
    if (accumulator >= tickInterval) {
        accumulator -= tickInterval;
        totalTicks++;

        seconds += 10;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                    days++;
                    if (days > 30) {
                        days = 1;
                        seasons++;
                        if (seasons > 4) {
                            seasons = 1;
                            years++;
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

uint64_t WorldClock::getTotalTicks() const { return totalTicks; }
int WorldClock::getSeconds() const { return seconds; }
int WorldClock::getMinutes() const { return minutes; }
int WorldClock::getHours() const { return hours; }
int WorldClock::getDays() const { return days; }
int WorldClock::getSeasons() const { return seasons; }
int WorldClock::getYears() const { return years; }

float WorldClock::getTimeOfDay() const {
    return (hours * 3600.0f + minutes * 60.0f + seconds) / 86400.0f;
}

}