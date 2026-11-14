#pragma once
#include <atomic>

namespace auth_input {
inline std::atomic<bool> blocked{false};
inline void SetBlocked(bool value) { blocked.store(value,std::memory_order_release); }
inline bool Blocked() { return blocked.load(std::memory_order_acquire); }
inline int TouchCount(int (*original)()) { return !Blocked() && original?original():0; }
}
