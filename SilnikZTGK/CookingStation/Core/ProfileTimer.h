#pragma once
#include <chrono>

class ProfileTimer {
public:
    ProfileTimer(float& outTime) : m_OutTime(outTime) {
        m_StartTime = std::chrono::high_resolution_clock::now();
    }
    ~ProfileTimer() {
        auto endTime = std::chrono::high_resolution_clock::now();
        m_OutTime = std::chrono::duration<float, std::chrono::milliseconds::period>(endTime - m_StartTime).count();
    }
private:
    float& m_OutTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
};