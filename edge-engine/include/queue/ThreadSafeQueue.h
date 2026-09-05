#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(item);
        }

        condition_.notify_one();
    }

    T wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait(lock, [this] {
            return !queue_.empty();
        });

        T item = queue_.front();
        queue_.pop();

        return item;
    }

    std::optional<T> wait_and_pop(const std::atomic<bool>& stop_flag) {
        std::unique_lock<std::mutex> lock(mutex_);

        condition_.wait_for(lock, std::chrono::milliseconds(100), [this,
            &stop_flag] {
            return !queue_.empty() || stop_flag.load();
        });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T item = queue_.front();
        queue_.pop();
        return item;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
};
