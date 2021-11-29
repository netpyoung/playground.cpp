#pragma once

namespace nf::network::packet
{
	// writer
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(THeader const& v)
	{
		assert(mWritePos == mBodyBegin);
		SetHeader(header);
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(int8_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<int8_t*>(&v)), sizeof(int8_t));
		assert(putResult == sizeof(int8_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(int16_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<int16_t*>(&v)), sizeof(int16_t));
		assert(putResult == sizeof(int16_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(int32_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<int32_t*>(&v)), sizeof(int32_t));
		assert(putResult == sizeof(int32_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(int64_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<int64_t*>(&v)), sizeof(int64_t));
		assert(putResult == sizeof(int64_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(uint8_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<uint8_t*>(&v)), sizeof(uint8_t));
		assert(putResult == sizeof(uint8_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(uint16_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<uint16_t*>(&v)), sizeof(uint16_t));
		assert(putResult == sizeof(uint16_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(uint32_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<uint32_t*>(&v)), sizeof(uint32_t));
		assert(putResult == sizeof(uint32_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(uint64_t const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<uint64_t*>(&v)), sizeof(uint64_t));
		assert(putResult == sizeof(uint64_t));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(std::byte const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<std::byte*>(&v)), sizeof(std::byte));
		assert(putResult == sizeof(std::byte));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(float const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<float*>(&v)), sizeof(float));
		assert(putResult == sizeof(float));
		return *this;
	}
	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	Packet<THeader, tHeaderSize, tBodyCapacity>& Packet<THeader, tHeaderSize, tBodyCapacity>::operator<<(double const v)
	{
		size_t const putResult = PutBody(reinterpret_cast<std::byte*>(const_cast<double*>(&v)), sizeof(double));
		assert(putResult == sizeof(double));
		return *this;
	}
} // namespace nf::network::packet
