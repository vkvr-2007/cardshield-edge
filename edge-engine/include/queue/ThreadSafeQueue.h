#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class ThreadSafeQueue {
public:
    void push(const T& item);

        T wait_and_pop();

            bool empty();

            private:
                std::queue<T> queue_;
                    mutable std::mutex mutex_;
                        std::condition_variable condition_;
                        };