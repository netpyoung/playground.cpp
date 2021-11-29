#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <algorithm>
#include <memory>
#pragma warning(pop)

#include <nf/objectpool/ObjectPoolLockFree.hpp>
#include <nf/objectpool/ObjectPoolTLS.hpp>
#include "nf/memory/eAllocationPolicy.hpp"

namespace nf::network::packet
{
	void PacketEncode(uint8_t const key, uint8_t const randKey, std::byte* inOutBytes, size_t const size);
	void PacketDecode(uint8_t const key, uint8_t const randKey, std::byte* inOutBytes, size_t const size);
	uint8_t CalculateCheckSum(std::byte* bytes, size_t const size);

	constexpr size_t PACKET_DEFAULT_BUFFER_SIZE = 4096;

	template<typename THeader, size_t tHeaderSize, size_t tBodyCapacity>
	class Packet
	{
	public:
		explicit Packet();
		explicit Packet(size_t const bufferSize);
		// explicit Packet(size_t const bufferSize, THeader const& header);
		explicit Packet(Packet const& packet) = delete;
		virtual ~Packet();
		Packet& operator=(Packet const&) = delete;

		void Initial(size_t bufferSize = PACKET_DEFAULT_BUFFER_SIZE);
		void ClearBuffer();
		bool EncodeOnce(uint8_t const code, uint8_t const key);
		bool DoDecode(uint8_t const key, uint8_t const randKey);


		size_t MoveBodyWritePos(size_t const size);
		size_t MoveBodyReadPos(size_t const size);
		void SetHeader(THeader const& header);
		void GetHeader(THeader* const header);
		
		size_t GetBody(std::byte* const data, size_t const size);
		size_t PeekBody(std::byte* const data, size_t const size);
		size_t PutBody(std::byte const* const data, size_t const size);
		
	public:
		// reader
		Packet& operator>>(THeader* const v);
		Packet& operator>>(int8_t* const v);
		Packet& operator>>(int16_t* const v);
		Packet& operator>>(int32_t* const v);
		Packet& operator>>(int64_t* const v);
		Packet& operator>>(uint8_t* const v);
		Packet& operator>>(uint16_t* const v);
		Packet& operator>>(uint32_t* const v);
		Packet& operator>>(uint64_t* const v);
		Packet& operator>>(std::byte* const v);
		Packet& operator>>(float* const v);
		Packet& operator>>(double* const v);

	public:
		// writer
		Packet& operator<<(THeader const& v);
		Packet& operator<<(int8_t const v);
		Packet& operator<<(int16_t const v);
		Packet& operator<<(int32_t const v);
		Packet& operator<<(int64_t const v);
		Packet& operator<<(uint8_t const v);
		Packet& operator<<(uint16_t const v);
		Packet& operator<<(uint32_t const v);
		Packet& operator<<(uint64_t const v);
		Packet& operator<<(std::byte const v);
		Packet& operator<<(float const v);
		Packet& operator<<(double const v);
	public:
		constexpr static size_t GetHeaderSize()
		{
			return tHeaderSize;
		}

		constexpr static size_t GetBodyCapacity()
		{
			return tBodyCapacity;
		}

		inline size_t GetPacketSize() const
		{
			return GetHeaderSize() + GetBodySize();
		}

		inline size_t GetBodySize() const
		{
			return mWritePos - mBodyBegin;
		}

		inline std::byte* GetBodyPtr() const
		{
			return mBodyBegin;
		}

		inline std::byte* GetHeaderPtr() const
		{
			return const_cast<std::byte*>(mBuffer);
		}

		inline int64_t AddRef()
		{
			return InterlockedIncrement64(&mRefCount);
		}

		inline int64_t AddRef(int64_t const x)
		{
			return InterlockedAdd64(&mRefCount, x);
		}

		inline int64_t SubRef()
		{
			return InterlockedDecrement64(&mRefCount);
		}

		inline int64_t RefCount() const
		{
			return mRefCount;
		}

	protected:

		//    mReadPos/mWritePos
		//           |
		// mHeader mBuffer
		//  |        |
		//  |-Header-|----Body---|
		//  |        |           |
		//        mBodyBegin  mBodyEnd
		std::byte mBuffer[tHeaderSize + tBodyCapacity];

		std::byte* mReadPos{ nullptr };
		std::byte* mWritePos{ nullptr };

		std::byte* mBodyBegin{ nullptr };
		std::byte* mBodyEnd{ nullptr };
		std::byte* mEncodeBegin{ nullptr };
		volatile uint32_t mIsEncoded{ 0 };
		volatile int64_t mRefCount{ 0 };
	
	};
} // namespace nf::network::packet

#include "Packet_impl.hpp"
#include "Packet_reader.hpp"
#include "Packet_writer.hpp"