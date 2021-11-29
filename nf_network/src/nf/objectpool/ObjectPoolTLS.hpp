#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <tuple>
#include <malloc.h>
#include <thread>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "nf/memory/eAllocationPolicy.hpp"
#include "ObjectPoolLockFree.hpp"

namespace nf::objectpool
{
	constexpr size_t MAX_BLOCK_BUFFER_SIZE = 100;
	constexpr int64_t TLS_CHUNK_GUARD = 0x0ABCDEF123456789;

	template <typename T, memory::eAllocationPolicy tPolicy>
	class ObjectPoolTLS final
	{
		struct ObjectPoolTLSChunk
		{
		public:
#pragma pack(push, 1)
			struct BlockNode
			{
				ObjectPoolTLSChunk* OriginChunk{ nullptr };
				int64_t SafeUpper{ TLS_CHUNK_GUARD }; // 메모리 탐지버퍼.
				T Data;
				int64_t SafeLower{ TLS_CHUNK_GUARD }; // 메모리 탐지버퍼.
			};
#pragma pack(pop)

		public:
			ObjectPoolTLSChunk()
			{
				for (auto& blockBuffer : BlockBuffer)
				{
					blockBuffer.OriginChunk = this;
				}
			}
			~ObjectPoolTLSChunk() = default;
		public:
			static inline BlockNode* GetBlockNodePtr(T* const ptrT)
			{
				return reinterpret_cast<BlockNode*>(reinterpret_cast<std::byte*>(ptrT) - sizeof(int64_t) - sizeof(ObjectPoolTLSChunk*));
			}

			std::array<BlockNode, MAX_BLOCK_BUFFER_SIZE> BlockBuffer;
			int64_t BufferAllocIndex{ static_cast<int64_t>(MAX_BLOCK_BUFFER_SIZE) };
			int64_t AvailableFreeCount{ static_cast<int64_t>(MAX_BLOCK_BUFFER_SIZE) };
		};
	public:
		explicit ObjectPoolTLS();
		explicit ObjectPoolTLS(size_t const initialCapacity);
		virtual ~ObjectPoolTLS();
		ObjectPoolTLS(ObjectPoolTLS& other) = delete;
		ObjectPoolTLS& operator=(ObjectPoolTLS  const& other) = delete;
	public:
		void Init(size_t const blockCount);
		void Clear(bool const isForce);

		template<typename... Args>
		T* Rent(Args&&... args);

		bool Return(T* const ptrT);

	public:
		inline size_t GetMaxCapacity() const
		{
			return mChunkPool.GetMaxCapacity() * MAX_BLOCK_BUFFER_SIZE;
		}

		inline int64_t GetUsedSize() const
		{
			return mTLSAllocatedSize;
		}
	private:
		ObjectPoolLockFree<ObjectPoolTLSChunk, memory::eAllocationPolicy::CallConstructorDestructor> mChunkPool;
		memory::eAllocationPolicy mPolicy{ tPolicy };
		::DWORD mTlsIndexForChunk;
		volatile int64_t mTLSAllocatedSize{ 0 };

	};

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolTLS<T, tPolicy>::ObjectPoolTLS()
	{
		mTlsIndexForChunk = ::TlsAlloc();
		if (mTlsIndexForChunk == TLS_OUT_OF_INDEXES)
		{
			throw std::bad_alloc();
		}
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolTLS<T, tPolicy>::ObjectPoolTLS(size_t const /*initialCapacity*/)
	{
		mTlsIndexForChunk = ::TlsAlloc();
		if (mTlsIndexForChunk == TLS_OUT_OF_INDEXES)
		{
			throw std::bad_alloc();
		}
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolTLS<T, tPolicy>::~ObjectPoolTLS()
	{
		ObjectPoolTLSChunk* chunk = reinterpret_cast<ObjectPoolTLSChunk*>(::TlsGetValue(mTlsIndexForChunk));
		if (chunk != nullptr)
		{
			mChunkPool.Return(chunk);
		}
		::TlsFree(mTlsIndexForChunk);
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	template<typename... Args>
	T* ObjectPoolTLS<T, tPolicy>::Rent(Args&&... args)
	{
		// Single Rent & Multiple Return
		ObjectPoolTLSChunk* chunk = reinterpret_cast<ObjectPoolTLSChunk*>(::TlsGetValue(mTlsIndexForChunk));

		if (chunk == nullptr)
		{
			chunk = mChunkPool.Rent();
			if (!::TlsSetValue(mTlsIndexForChunk, reinterpret_cast<LPVOID>(chunk)))
			{
				throw std::logic_error("Could't TlsSetValue()");
			}
		}
		
		chunk->BufferAllocIndex--;
		
		if (chunk->BufferAllocIndex == 0)
		{
			if (!::TlsSetValue(mTlsIndexForChunk, reinterpret_cast<LPVOID>(mChunkPool.Rent())))
			{
				throw std::logic_error("Could't TlsSetValue()");
			}
		}

		ObjectPoolTLSChunk::BlockNode& blockNode = chunk->BlockBuffer[chunk->BufferAllocIndex];

		T* const t = &blockNode.Data;

		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			new (t) T(std::forward<Args>(args)...);
		}

		::InterlockedIncrement64(&mTLSAllocatedSize);

		return t;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	bool ObjectPoolTLS<T, tPolicy>::Return(T* const ptrT)
	{
		// Single Rent & Multiple Return

		ObjectPoolTLSChunk::BlockNode* const blockNode = ObjectPoolTLSChunk::GetBlockNodePtr(ptrT);
		assert(blockNode != nullptr);
		assert(blockNode->SafeUpper == TLS_CHUNK_GUARD);
		assert(blockNode->SafeLower == TLS_CHUNK_GUARD);

		ObjectPoolTLSChunk* const chunk = blockNode->OriginChunk;
		assert(chunk != nullptr);
		assert(chunk->AvailableFreeCount >= 0);

		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			ptrT->~T();
		}

		int64_t const freeCount = InterlockedDecrement64(&chunk->AvailableFreeCount);
		assert(freeCount >= 0);
		if (freeCount == 0)
		{
			assert(chunk->BufferAllocIndex == 0);
			mChunkPool.Return(chunk);
		}

		::InterlockedDecrement64(&mTLSAllocatedSize);
		return true;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	void ObjectPoolTLS<T, tPolicy>::Init(size_t const blockCount)
	{

	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	void ObjectPoolTLS<T, tPolicy>::Clear(bool const /*isForce*/)
	{
	}

} //namespace nf::objectpool
