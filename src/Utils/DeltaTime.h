#ifndef DELTATIME_H
#define DELTATIME_H

#include <chrono>

#include "Singleton.h"

class DeltaTime final: public Singleton<DeltaTime> {
public:
    DeltaTime(const DeltaTime& other) = delete;
    DeltaTime(DeltaTime&& other) = delete;
    DeltaTime& operator=(const DeltaTime& other) = delete;
    DeltaTime& operator=(DeltaTime&& other) = delete;

    [[nodiscard]] double GetFixedDeltaTime() const;
    [[nodiscard]] double GetDeltaTime() const;

    [[nodiscard]] std::chrono::nanoseconds SleepDuration() const;

    void Update();

private:
    friend class Singleton<DeltaTime>;

    DeltaTime() = default;
    static constexpr double m_FixedDeltaTime{1.0 / 60.0};
    static constexpr double FPS{60};
    double m_DeltaTime{};
    std::chrono::high_resolution_clock::time_point m_PrevTime;
};



#endif //DELTATIME_H
