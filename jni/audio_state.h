#pragma once
#include <atomic>
struct AudioState {
    std::atomic<float> rpm{0.12f};
    std::atomic<float> throttle{0.f};
    std::atomic<float> speed01{0.f};
    std::atomic<float> slip{0.f};
    std::atomic<int> profile{1};
    std::atomic<bool> enabled{true};
    std::atomic<bool> running{false};
};
AudioState& audioState();
void audioStart(); void audioStop(); void audioOneshot(int t);
