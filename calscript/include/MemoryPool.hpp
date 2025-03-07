#pragma once
#include <vector>
#include <memory>
#include <cstddef>
#include <array>

namespace calc {
    template<typename T, size_t BlockSize = 4096>
    class MemoryPool {
    private:
        struct Block {
            std::array<std::byte, BlockSize> data;
            size_t used = 0;
            std::unique_ptr<Block> next = nullptr;
        };

        std::unique_ptr<Block> head_;
        Block* current_;

    public:
        MemoryPool() : head_(std::make_unique<Block>()), current_(head_.get()) {}

        template<typename... Args>
        T* allocate(Args&&... args) {
            constexpr size_t size = sizeof(T);
            constexpr size_t alignment = alignof(T);

            // Align the current position
            size_t adjustment = (alignment - (current_->used % alignment)) % alignment;
            size_t newUsed = current_->used + adjustment + size;

            if (newUsed > BlockSize) {
                // Create new block if current is full
                if (!current_->next) {
                    current_->next = std::make_unique<Block>();
                }
                current_ = current_->next.get();
                current_->used = 0;
                adjustment = (alignment - (current_->used % alignment)) % alignment;
                newUsed = adjustment + size;
            }

            // Get properly aligned address
            void* addr = &current_->data[current_->used + adjustment];
            current_->used = newUsed;

            // Construct object in-place
            return new (addr) T(std::forward<Args>(args)...);
        }

        void reset() {
            current_ = head_.get();
            while (current_) {
                current_->used = 0;
                current_ = current_->next.get();
            }
            current_ = head_.get();
        }
    };
}