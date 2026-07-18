#pragma once

class WorldClock {
private:
    float totalTime;
    float timeMultiplier;
    bool isPaused;
    int days;
    int seasons;
    int years;
    float dayLength;

public:
    WorldClock();
    void update(float dt);
    void setMultiplier(float mult);
    void togglePause();
    float getTimeOfDay() const;
    int getDay() const;
    int getSeason() const;
    int getYear() const;
    float getTotalTime() const;
    void setTotalTime(float time);
    bool getPaused() const;
};