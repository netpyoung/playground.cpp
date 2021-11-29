#pragma once

namespace nf::network::packet
{
	// reader
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(THeader* const v)
	{
		assert(mReadPos == mBodyBegin);
		GetHeader(v);
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(int8_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(int8_t));
		assert(getResult == sizeof(int8_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(int16_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(int16_t));
		assert(getResult == sizeof(int16_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(int32_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(int32_t));
		assert(getResult == sizeof(int32_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(int64_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(int64_t));
		assert(getResult == sizeof(int64_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(uint8_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(uint8_t));
		assert(getResult == sizeof(uint8_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(uint16_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(uint16_t));
		assert(getResult == sizeof(uint16_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(uint32_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(uint32_t));
		assert(getResult == sizeof(uint32_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(uint64_t* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(uint64_t));
		assert(getResult == sizeof(uint64_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(std::byte* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(std::byte));
		assert(getResult == sizeof(std::byte));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(float* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(float));
		assert(getResult == sizeof(float));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator>>(double* const v)
	{
		size_t const getResult = GetBody(reinterpret_cast<std::byte*>(v), sizeof(double));
		assert(getResult == sizeof(double));
		return *this;
	}
} // namespace nf::network::packet
