#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
#include <memory>
#include <algorithm> // std::min
#include <cassert>
#pragma warning(pop)

namespace nf::buffer::ringbuffer
{
	class RingBuffer
	{
	public:
		RingBuffer() = default;
		RingBuffer(size_t const maxCapacity);
		virtual ~RingBuffer() = default;
	public:
		RingBuffer(RingBuffer const& other) = delete;
		RingBuffer& operator=(RingBuffer const& other) = delete;
	public:
		void Init(size_t const capacity);
		void ClearBuffer();
		size_t Enqueue(std::byte const* const bytes, size_t const size);
		size_t Dequeue(std::byte* const bytes, size_t const size);
		size_t Peek(std::byte* const bytes, size_t const size);
		size_t MoveRead(size_t const size);
		size_t MoveWrite(size_t const size);

	public:
		inline size_t GetMaxCapacity() const
		{
			return mMaxCapacity;
		}

		inline size_t GetUsedSize() const
		{
			return mUsedSize;
		}

		inline size_t GetEnquableSize() const
		{
			return mMaxCapacity - mUsedSize;
		}

		inline bool IsEmpty() const
		{
			return mUsedSize == 0;
		}

		inline size_t GetDirectEnqueueSize() const
		{
			if (GetEnquableSize() == 0)
			{
				return 0;
			}

			if (mReadIndex <= mWriteIndex)
			{
				return mMaxCapacity - mWriteIndex;
			}
			else
			{
				return mReadIndex - mWriteIndex;
			}
		}

		inline size_t GetDirectDequeueSize() const
		{
			if (IsEmpty())
			{
				return 0;
			}

			if (mReadIndex <= mWriteIndex)
			{
				return mWriteIndex - mReadIndex;
			}
			else
			{
				return mMaxCapacity - mReadIndex;
			}
		}

		inline std::byte* GetReadBufferPtr() const
		{
			return mBegin + mReadIndex;
		}

		inline std::byte* GetWriteBufferPtr() const
		{
			return mBegin + mWriteIndex;
		}

		inline std::byte* GetBeginBufferPtr() const
		{
			return mBegin;
		}
	private:
		std::unique_ptr<std::byte[]> mBuffer{ nullptr };
		std::byte* mBegin{ nullptr };
		size_t mMaxCapacity{ 0 };
		size_t mUsedSize{ 0 };
		size_t mWriteIndex{ 0 };
		size_t mReadIndex{ 0 };
	};
} // namespace nf::buffer::ringbuffer