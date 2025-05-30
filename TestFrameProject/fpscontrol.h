#pragma once
#include <chrono>
#include <thread>
#include <iostream>
#include <cstdint>

class FPS {
public:
    FPS() = delete;

    explicit FPS(uint64_t fps)
        : m_MicrosecondsPerFrame(1000000 / fps),
        m_last_time(std::chrono::steady_clock::now())
    {
    }

    uint64_t CalcDelta() {
        auto now = std::chrono::steady_clock::now();
        auto delta_us = std::chrono::duration_cast<std::chrono::microseconds>(now - m_last_time).count();
        m_last_time = now;
        m_delta_time = delta_us;
        return m_delta_time;
    }

    void Wait() const {
        int64_t sleep_us = static_cast<int64_t>(m_MicrosecondsPerFrame) - static_cast<int64_t>(m_delta_time);
        if (sleep_us > 0) {
#if defined(DEBUG) || defined(_DEBUG)
            std::cout << "Sleep time: " << sleep_us / 1000.0f << " ms" << std::endl;
#endif
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_us));
        }
    }

private:
    uint64_t m_MicrosecondsPerFrame = 0;
    uint64_t m_delta_time = 0;
    std::chrono::steady_clock::time_point m_last_time;
};
