#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <string>

namespace vov {
    class Timer {
    public:
        explicit Timer(std::string  taskName = "");

        ~Timer();
        void stop();
        [[nodiscard]] long long elapsedMilliseconds() const;

    private:
        std::string m_taskName;
        std::chrono::high_resolution_clock::time_point m_start;
        std::chrono::milliseconds m_duration{0};
        bool m_running;
    };
}

#endif //TIMER_H
