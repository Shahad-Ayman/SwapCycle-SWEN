#pragma once

// Fixed-window, in-memory attempt counter used to rate-limit login,
// registration and password-reset requests. Thread-safe.
// In-memory is enough for the single-instance deployment; a shared store
// (e.g. Redis) would be needed if the backend is scaled out.

#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
#include <unordered_map>

namespace swapcycle::accounts {

class RateLimiter {
public:
    using Clock = std::chrono::steady_clock;

    RateLimiter(std::size_t maxAttempts, std::chrono::seconds window);

    // Records one attempt for `key`. Returns 0 if it is allowed, otherwise the
    // number of seconds until the key may try again.
    std::chrono::seconds hit(const std::string& key, Clock::time_point now = Clock::now());

    void reset(const std::string& key);

private:
    struct Window {
        Clock::time_point start;
        std::size_t attempts = 0;
    };

    void pruneExpired(Clock::time_point now);

    const std::size_t maxAttempts_;
    const std::chrono::seconds window_;
    std::mutex mutex_;
    std::unordered_map<std::string, Window> windows_;
};

}  // namespace swapcycle::accounts
