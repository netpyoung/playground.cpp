#include "RingBuffer.hpp"

namespace nf::buffer::ringbuffer
{
	RingBuffer::RingBuffer(size_t const maxCapacity)
	{
		Init(maxCapacity);
	}

	void RingBuffer::Init(size_t const maxCapacity)
	{
		mBuffer = std::make_unique<std::byte[]>(maxCapacity + 1);
		mMaxCapacity = maxCapacity;
		mBegin = mBuffer.get();
		mUsedSize = 0;
		mWriteIndex = 0;
		mReadIndex = 0;
	}

	size_t RingBuffer::Enqueue(std::byte const* const bytes, size_t const size)
	{
		if (size == 0)
		{
			return 0;
		}

		size_t const available = std::min(size, mMaxCapacity - mUsedSize);
		if (available <= mMaxCapacity - mWriteIndex)
		{
			memcpy(mBegin + mWriteIndex, bytes, available);
			mWriteIndex += available;
			if (mWriteIndex == mMaxCapacity)
			{
				mWriteIndex = 0;
			}
		}
		else
		{
			size_t const fst = mMaxCapacity - mWriteIndex;
			size_t const snd = available - fst; 
			memcpy(mBegin + mWriteIndex, bytes, fst);
			memcpy(mBegin, bytes + fst, snd);
			mWriteIndex = snd;
		}
		mUsedSize += available;
		return available;
	}

	size_t RingBuffer::Dequeue(std::byte* const bytes, size_t const size)
	{
		if (size == 0)
		{
			return 0;
		}

		size_t const available = std::min(size, mUsedSize);
		if (available <= mMaxCapacity - mReadIndex)
		{
			memcpy(bytes, mBegin + mReadIndex, available);
			mReadIndex += available;
			if (mReadIndex == mMaxCapacity)
			{
				mReadIndex = 0;
			}
		}
		else
		{
			size_t const fst = mMaxCapacity - mReadIndex;
			size_t const snd = available - fst;
			memcpy(bytes, mBegin + mReadIndex, fst);
			memcpy(bytes + fst, mBegin, snd);
			mReadIndex = snd;
		}

		mUsedSize -= available;
		return available;
	}

	size_t RingBuffer::Peek(std::byte* const bytes, size_t const size)
	{
		if (size == 0)
		{
			return 0;
		}

		size_t const available = std::min(size, mUsedSize);
		if (available <= mMaxCapacity - mReadIndex)
		{
			memcpy(bytes, mBegin + mReadIndex, available);
		}
		else
		{
			size_t const fst = mMaxCapacity - mReadIndex;
			size_t const snd = available - fst;
			memcpy(bytes, mBegin + mReadIndex, fst);
			memcpy(bytes + fst, mBegin, snd);
		}

		return available;
	}

	size_t RingBuffer::MoveWrite(size_t const size)
	{
		if (size == 0)
		{
			return 0;
		}

		size_t const available = std::min(size, mMaxCapacity - mUsedSize);
		if (available <= mMaxCapacity - mWriteIndex)
		{
			mWriteIndex += available;
			if (mWriteIndex == mMaxCapacity)
			{
				mWriteIndex = 0;
			}
		}
		else
		{
			mWriteIndex = available - (mMaxCapacity - mWriteIndex);
		}
		mUsedSize += available;
		return available;
	}

	size_t RingBuffer::MoveRead(size_t const size)
	{
		if (size == 0)
		{
			return 0;
		}

		size_t const available = std::min(size, mUsedSize);
		if (available <= mMaxCapacity - mReadIndex)
		{
			mReadIndex += available;
			if (mReadIndex == mMaxCapacity)
			{
				mReadIndex = 0;
			}
		}
		else
		{
			mReadIndex = available - (mMaxCapacity - mReadIndex);
		}

		mUsedSize -= available;
		return available;
	}


	void RingBuffer::ClearBuffer()
	{
		mUsedSize = 0;
		mWriteIndex = 0;
		mReadIndex = 0;
	}
} // namespace nf::buffer::ringbuffer