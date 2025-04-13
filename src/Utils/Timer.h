#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <iostream>
#include <string>

#include "Chalk.h"

class Timer {
public:
    explicit Timer(const std::string& taskName = "")
        : m_taskName(taskName), m_start(std::chrono::high_resolution_clock::now()), m_running(true) {}

    void stop() {
        if (!m_running) return;
        auto end = std::chrono::high_resolution_clock::now();
        m_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        m_running = false;

        if (!m_taskName.empty()) {
            std::cout << m_taskName << " took " << Chalk::Blue <<  m_duration.count() << Chalk::Reset << " ms\n";
        }
    }

    ~Timer() {
        stop();
    }

    [[nodiscard]] long long elapsedMilliseconds() const {
        return m_duration.count();
    }

private:
    std::string m_taskName;
    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::milliseconds m_duration{0};
    bool m_running;
};

#endif //TIMER_H
