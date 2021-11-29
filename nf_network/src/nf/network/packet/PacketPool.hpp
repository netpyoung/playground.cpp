#pragma once
#include "Packet.hpp"
#define NF_SUPPORT_TLS 1

namespace nf::network::packet
{
	template <typename TPacket>
	class PacketPool
	{
	public:
		static bool InitSendPool(size_t count);
		static bool InitRecvPool(size_t count);

		static TPacket* AllocSend();
		static TPacket* AllocRecv();

		static bool TryFreeSend(TPacket* const packet);
		static bool TryFreeRecv(TPacket* const packet);
		static void Clear();

	public:
		inline static size_t GetSendMaxCapacity()
		{
			return sSendMemoryPool->GetMaxCapacity();
		}
		inline static size_t GetSendUsedSize()
		{
			return sSendMemoryPool->GetUsedSize();
		}

		inline static size_t GetRecvMaxCapacity()
		{
			return sRecvMemoryPool->GetMaxCapacity();
		}
		inline static size_t GetRecvUsedSize()
		{
			return sRecvMemoryPool->GetUsedSize();
		}
	private:
#if NF_SUPPORT_TLS
		static std::unique_ptr<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> sSendMemoryPool;
		static std::unique_ptr<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> sRecvMemoryPool;
#else
		static std::unique_ptr<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> sSendMemoryPool;
		static std::unique_ptr<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> sRecvMemoryPool;
#endif

	};
#if NF_SUPPORT_TLS
	template<typename TPacket>
	std::unique_ptr<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> PacketPool<TPacket>::sSendMemoryPool = nullptr;
	template<typename TPacket>
	std::unique_ptr<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> PacketPool<TPacket>::sRecvMemoryPool = nullptr;
#else
	template<typename TPacket>
	std::unique_ptr<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> PacketPool<TPacket>::sSendMemoryPool = nullptr;
	template<typename TPacket>
	std::unique_ptr<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>> PacketPool<TPacket>::sRecvMemoryPool = nullptr;
#endif // NF_SUPPORT_TLS

	template <typename TPacket>
	bool PacketPool<TPacket>::InitSendPool(size_t count)
	{
		if (sSendMemoryPool != nullptr)
		{
			return false;
		}

#if NF_SUPPORT_TLS
		sSendMemoryPool = std::make_unique<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>>(count);
#else
		sSendMemoryPool = std::make_unique<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>>(count);
#endif // NF_SUPPORT_TLS
		return true;
	}

	template <typename TPacket>
	bool PacketPool<TPacket>::InitRecvPool(size_t count)
	{
		if (sRecvMemoryPool != nullptr)
		{
			return false;
		}

#if NF_SUPPORT_TLS
		sRecvMemoryPool = std::make_unique<objectpool::ObjectPoolTLS<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>>(count);
#else
		sRecvMemoryPool = std::make_unique<objectpool::ObjectPoolLockFree<TPacket, memory::eAllocationPolicy::CallConstructorDestructor>>(count);
#endif // sRecvMemoryPool
		return true;
	}

	template <typename TPacket>
	TPacket* PacketPool<TPacket>::AllocSend()
	{
		TPacket* const packet = sSendMemoryPool->Rent();
		int64_t const refCount = packet->AddRef();
		assert(refCount == 1);
		return packet;
	}
	template <typename TPacket>
	TPacket* PacketPool<TPacket>::AllocRecv()
	{
		TPacket* const packet = sRecvMemoryPool->Rent();
		int64_t const refCount = packet->AddRef();
		assert(refCount == 1);
		return packet;
	}
	template <typename TPacket>
	bool PacketPool<TPacket>::TryFreeSend(TPacket* const packet)
	{
		if (packet->SubRef() == 0)
		{
			return sSendMemoryPool->Return(packet);
		}
		return false;
	}

	template <typename TPacket>
	bool PacketPool<TPacket>::TryFreeRecv(TPacket* const packet)
	{
		if (packet->SubRef() == 0)
		{
			return sRecvMemoryPool->Return(packet);
		}
		return false;
	}

	template <typename TPacket>
	void PacketPool<TPacket>::Clear()
	{
		if (sSendMemoryPool != nullptr)
		{
			sSendMemoryPool->Clear(true);
		}
		if (sRecvMemoryPool != nullptr)
		{
			sRecvMemoryPool->Clear(true);
		}
	}
} // namespace nf::network::packet
