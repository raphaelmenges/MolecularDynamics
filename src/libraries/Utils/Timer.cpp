//
// Created by ubundrian on 22.08.16.
//

#include "Timer.h"

void Timer::start()
{
    startTime = std::chrono::high_resolution_clock::now();
    endTime = startTime;
}

void Timer::stop()
{
    endTime = std::chrono::high_resolution_clock::now();
    long duration = std::chrono::duration_cast<std::chrono::microseconds>( endTime - startTime ).count();
    durations.push_back(duration);
    totalDuration += duration;
    if (durations.size() >= limit) {
        long oldestDuration = durations.front();
        totalDuration -= oldestDuration;
        durations.pop_front();
    }
}

double Timer::getDuration()
{
    return totalDuration / durations.size();
}