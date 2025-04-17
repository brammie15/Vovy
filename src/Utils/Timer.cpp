#include "Timer.h"

#include <iostream>

#include "Chalk.h"

namespace vov {
    void Timer::stop() {
        if (!m_running) return;
        const auto end = std::chrono::high_resolution_clock::now();
        m_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start);
        m_running = false;

        if (!m_taskName.empty()) {
            std::cout << m_taskName << " took " << Chalk::Blue << m_duration.count() << Chalk::Reset << " ms\n";
        }
    }

    Timer::Timer(std::string taskName): m_taskName(std::move(taskName)), m_start(std::chrono::high_resolution_clock::now()), m_running(true) {
    }

    Timer::~Timer() {
        stop();
    }

    long long Timer::elapsedMilliseconds() const {
        return m_duration.count();
    }
}
