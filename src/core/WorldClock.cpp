#include "core/WorldClock.h"

WorldClock::WorldClock() {
    totalTime = 0.f;
    timeMultiplier = 1.0f;
    isPaused = false;
    dayLength = 1200.f; 
    days = 1;
    seasons = 1;
    years = 1;
}

void WorldClock::update(float dt) {
    if (isPaused) return;

    totalTime += dt * timeMultiplier;
    
    int newDays = static_cast<int>(totalTime / dayLength) + 1;
    if (newDays > days) {
        days = newDays;
        if (days % 15 == 0) seasons++; 
        if (seasons > 4) {
            seasons = 1;
            years++;
        }
    }
}

void WorldClock::setMultiplier(float mult) {
    timeMultiplier = mult;
}

void WorldClock::togglePause() {
    isPaused = !isPaused;
}

float WorldClock::getTimeOfDay() const {
    float currentDayTime = totalTime - ((days - 1) * dayLength);
    return currentDayTime / dayLength; 
}

int WorldClock::getDay() const { return days; }
int WorldClock::getSeason() const { return seasons; }
int WorldClock::getYear() const { return years; }
float WorldClock::getTotalTime() const { return totalTime; }
void WorldClock::setTotalTime(float time) { totalTime = time; }
bool WorldClock::getPaused() const { return isPaused; }