#include "RateLimiter.h"

namespace swapcycle::accounts {

namespace {
constexpr std::size_t kPruneThreshold = 10'000;
}

RateLimiter::RateLimiter(std::size_t maxAttempts, std::chrono::seconds window)
    : maxAttempts_(maxAttempts), window_(window) {}

std::chrono::seconds RateLimiter::hit(const std::string& key, Clock::time_point now) {
    std::lock_guard lock(mutex_);
    if (windows_.size() > kPruneThreshold) pruneExpired(now);

    auto& entry = windows_[key];
    if (entry.attempts == 0 || now - entry.start >= window_) {
        entry = Window{now, 0};
    }
    if (entry.attempts >= maxAttempts_) {
        const auto remaining = std::chrono::ceil<std::chrono::seconds>(entry.start + window_ - now);
        return std::max(remaining, std::chrono::seconds{1});
    }
    ++entry.attempts;
    return std::chrono::seconds{0};
}

void RateLimiter::reset(const std::string& key) {
    std::lock_guard lock(mutex_);
    windows_.erase(key);
}

void RateLimiter::pruneExpired(Clock::time_point now) {
    for (auto it = windows_.begin(); it != windows_.end();) {
        it = (now - it->second.start >= window_) ? windows_.erase(it) : std::next(it);
    }
}

}  // namespace swapcycle::accounts
