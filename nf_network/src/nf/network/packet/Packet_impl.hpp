#pragma once
#include "PacketReaderException.hpp"

namespace nf::network::packet
{
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>::Packet() : Packet(PACKET_DEFAULT_BUFFER_SIZE)
	{
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>::Packet(size_t const bufferSize)
	{
		Initial(bufferSize);
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>::~Packet()
	{
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	void Packet<THeader, tHeaderSize, tBodyCapacity>::Initial(size_t const /*bufferSize*/)
	{
		mBodyBegin = mBuffer + tHeaderSize;
		mEncodeBegin = mBodyBegin - 1;
		mBodyEnd = mBuffer + tBodyCapacity;
		mReadPos = mBodyBegin;
		mWritePos = mBodyBegin;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	void Packet<THeader, tHeaderSize, tBodyCapacity>::ClearBuffer()
	{
		mReadPos = mBodyBegin;
		mWritePos = mBodyBegin;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>

	size_t Packet<THeader, tHeaderSize, tBodyCapacity>::MoveBodyWritePos(size_t const size)
	{
		size_t const moveSize = std::min(static_cast<size_t>(mBodyEnd - mWritePos), size);
		mWritePos += moveSize;
		return moveSize;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>

	size_t Packet<THeader, tHeaderSize, tBodyCapacity>::MoveBodyReadPos(size_t const size)
	{
		size_t const moveSize = std::min(static_cast<size_t>(mWritePos - mReadPos), size);
		mReadPos += moveSize;
		return moveSize;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	void Packet<THeader, tHeaderSize, tBodyCapacity>::SetHeader(THeader const& header)
	{
		memcpy(mBuffer, &header, tHeaderSize);
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	void Packet<THeader, tHeaderSize, tBodyCapacity>::GetHeader(THeader* const header)
	{
		memcpy(header, mBuffer, tHeaderSize);
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	size_t Packet<THeader, tHeaderSize, tBodyCapacity>::GetBody(std::byte* const data, size_t const size)
	{
		size_t const min = std::min((size_t)(mWritePos - mReadPos), size);
		if (min != size)
		{
			throw PacketReaderException();
		}
		switch (size)
		{
		case 0:
			return 0;
		case 1:
			*data = *mReadPos;
			break;
		case 2:
			*((uint16_t*)data) = *((uint16_t*)mReadPos);
			break;
		case 3:
			*((uint16_t*)data) = *((uint16_t*)mReadPos);
			*((data + 2)) = *(mReadPos + 2);
			break;
		case 4:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			break;
		case 5:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((data + 4)) = *(mReadPos + 4);
			break;
		case 6:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((uint16_t*)(data + 4)) = *((uint16_t*)(mReadPos + 4));
			break;
		case 7:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((uint16_t*)(data + 4)) = *((uint16_t*)(mReadPos + 4));
			*((data + 6)) = *(mReadPos + 6);
			break;
		case 8:
			*((uint64_t*)data) = *((uint64_t*)mReadPos);
			break;
		default:
			memcpy(data, mReadPos, size);
			break;
		}
		//memcpy(data, mReadPos, size);

		mReadPos += size;
		return size;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	size_t Packet<THeader, tHeaderSize, tBodyCapacity>::PeekBody(std::byte* const data, size_t const size)
	{
		size_t const min = std::min((size_t)(mWritePos - mReadPos), size);
		switch (min)
		{
		case 0:
			return 0;
		case 1:
			*data = *mReadPos;
			break;
		case 2:
			*((uint16_t*)data) = *((uint16_t*)mReadPos);
			break;
		case 3:
			*((uint16_t*)data) = *((uint16_t*)mReadPos);
			*((data + 2)) = *(mReadPos + 2);
			break;
		case 4:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			break;
		case 5:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((data + 4)) = *(mReadPos + 4);
			break;
		case 6:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((uint16_t*)(data + 4)) = *((uint16_t*)(mReadPos + 4));
			break;
		case 7:
			*((uint32_t*)data) = *((uint32_t*)mReadPos);
			*((uint16_t*)(data + 4)) = *((uint16_t*)(mReadPos + 4));
			*((data + 6)) = *(mReadPos + 6);
			break;
		case 8:
			*((uint64_t*)data) = *((uint64_t*)mReadPos);
			break;
		default:
			memcpy(data, mReadPos, min);
			break;
		}
		//memcpy(data, mReadPos, min);

		return min;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	size_t Packet<THeader, tHeaderSize, tBodyCapacity>::PutBody(std::byte const* const data, size_t const size)
	{
		size_t const min = std::min((size_t)(mBodyEnd - mWritePos), size);
		switch (min)
		{
		case 0:
			return 0;
		case 1:
			*mWritePos = *data;
			break;
		case 2:
			*((uint16_t*)mWritePos) = *((uint16_t*)data);
			break;
		case 3:
			*((uint16_t*)mWritePos) = *((uint16_t*)data);
			*((mWritePos + 2)) = *(data + 2);
			break;
		case 4:
			*((uint32_t*)mWritePos) = *((uint32_t*)data);
			break;
		case 5:
			*((uint32_t*)mWritePos) = *((uint32_t*)data);
			*((mWritePos + 4)) = *(data + 4);
			break;
		case 6:
			*((uint32_t*)mWritePos) = *((uint32_t*)data);
			*((uint16_t*)(mWritePos + 4)) = *((uint16_t*)(data + 4));
			break;
		case 7:
			*((uint32_t*)mWritePos) = *((uint32_t*)data);
			*((uint16_t*)(mWritePos + 4)) = *((uint16_t*)(data + 4));
			*((mWritePos + 6)) = *(data + 6);
			break;
		case 8:
			*((uint64_t*)mWritePos) = *((uint64_t*)data);
			break;
		default:
			memcpy(mWritePos, data, min);
			break;
		}
		//memcpy(mWritePos, data, min);
		mWritePos += min;
		return min;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	bool Packet<THeader, tHeaderSize, tBodyCapacity>::EncodeOnce(uint8_t const code, uint8_t const key)
	{
		if (InterlockedExchange(&mIsEncoded, 1) != 0)
		{
			return false;
		}

		uint8_t const randKey = static_cast<uint8_t>(::rand() % 255);

		// TODO(pyoung): refactoring
		uint8_t const checkSum = packet::CalculateCheckSum(GetBodyPtr(), GetBodySize());
		*(mEncodeBegin) = std::byte(checkSum);
		PacketEncode(key, randKey, mEncodeBegin, GetBodySize() + 1);
		*(GetHeaderPtr()) = std::byte(code);
		*(reinterpret_cast<uint16_t*>(GetHeaderPtr() + 1)) = static_cast<uint16_t>(GetBodySize());
		*(GetHeaderPtr() + 3) = std::byte(randKey);
		return true;
	}

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	bool Packet<THeader, tHeaderSize, tBodyCapacity>::DoDecode(uint8_t const key, uint8_t const randKey)
	{
		// body앞 1byte는 체크섬.
		PacketDecode(key, randKey, mEncodeBegin, GetBodySize() + 1);
		uint8_t const savedInHeaderCheckSum = static_cast<uint8_t>(*mEncodeBegin);
		uint8_t const bodyCheckSum = CalculateCheckSum(GetBodyPtr(), GetBodySize());
		return savedInHeaderCheckSum == bodyCheckSum;
	}
} // namespace nf::network::packet
