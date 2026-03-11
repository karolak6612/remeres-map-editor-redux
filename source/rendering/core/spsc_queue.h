#ifndef RME_RENDERING_CORE_SPSC_QUEUE_H_
#define RME_RENDERING_CORE_SPSC_QUEUE_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <optional>

template<typename T, size_t Capacity>
class SPSCQueue {
	static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");

public:
	bool try_push(T&& value)
	{
		const size_t current_tail = tail_.load(std::memory_order_relaxed);
		const size_t next_tail = (current_tail + 1) & (Capacity - 1);
		if (next_tail == head_.load(std::memory_order_acquire)) {
			return false;
		}

		buffer_[current_tail].emplace(std::move(value));
		tail_.store(next_tail, std::memory_order_release);
		return true;
	}

	std::optional<T> try_pop()
	{
		const size_t current_head = head_.load(std::memory_order_relaxed);
		if (current_head == tail_.load(std::memory_order_acquire)) {
			return std::nullopt;
		}

		auto value = std::move(buffer_[current_head]);
		buffer_[current_head].reset();
		head_.store((current_head + 1) & (Capacity - 1), std::memory_order_release);
		return value;
	}

	[[nodiscard]] bool empty() const
	{
		return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
	}

	void clear()
	{
		for (auto& slot : buffer_) {
			slot.reset();
		}
		head_.store(0, std::memory_order_release);
		tail_.store(0, std::memory_order_release);
	}

private:
	alignas(64) std::atomic<size_t> head_ {0};
	alignas(64) std::atomic<size_t> tail_ {0};
	std::array<std::optional<T>, Capacity> buffer_;
};

#endif
