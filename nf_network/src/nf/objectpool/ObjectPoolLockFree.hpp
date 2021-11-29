#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <memory>
#include <tuple>
#include "nf/memory/eAllocationPolicy.hpp"

namespace nf::objectpool
{
	constexpr int64_t LOCKFREE_BLOCKNODE_GUARD = 0x1234567890ABCDEF;

	template <typename T, memory::eAllocationPolicy tPolicy>
	class ObjectPoolLockFree final
	{
	public:
		explicit ObjectPoolLockFree();
		explicit ObjectPoolLockFree(size_t const initialCapacity);
		virtual ~ObjectPoolLockFree();
		ObjectPoolLockFree(ObjectPoolLockFree& other) = delete;
		ObjectPoolLockFree& operator=(ObjectPoolLockFree  const& other) = delete;
	public:
		void Init(size_t const blockCount);
		void Clear(bool const isForce);

		template<typename... Args>
		T* Rent(Args&&... args);

		bool Return(T* const ptrT);

	public:
		inline size_t GetMaxCapacity() const
		{
			return mMaxCapacity;
		}

		inline int64_t GetUsedSize() const
		{
			return mUsedSize;
		}

	private:
		// |Next*|SafeUpper|t|SafeLower|
		// SafeUpper/SafeLower : T를 감싸는 메모리 탐지버퍼. 생성자를 호출 안함으로 코드에서 `GUARD`로 채워줌.
#pragma pack(push, 1)
		struct BlockNode
		{
			BlockNode* Next;
			int64_t SafeUpper; // 메모리 탐지버퍼.
			T t;
			int64_t SafeLower; // 메모리 탐지버퍼.
		};
#pragma pack(pop)

#pragma pack(push, 1)
		struct alignas(16) Indexer
		{
			uint64_t UID;
			BlockNode* Node;
		};
#pragma pack(pop)

		size_t Count()
		{
			BlockNode* willRemoveNode = mHead.Node;

			size_t acc = 0;
			while (willRemoveNode != nullptr)
			{
				BlockNode* const next = willRemoveNode->Next;
				willRemoveNode = next;
				acc++;
			}
			return acc;
		}

		std::tuple<BlockNode* const, size_t const> AllocateFreeBlockNodes(size_t const blockCount) const;

	private:
		inline ObjectPoolLockFree <T, tPolicy>::BlockNode* GetPtrBlockNode(T* const ptrT) const
		{
			return reinterpret_cast<BlockNode*>(reinterpret_cast<std::byte*>(ptrT) - sizeof(int64_t) - sizeof(BlockNode*));
		}
	private:
		alignas(16) Indexer mHead;
		volatile size_t mMaxCapacity{ 0 };
		volatile int64_t mCachedSize{ 0 };
		volatile int64_t mUsedSize{ 0 };
		memory::eAllocationPolicy mPolicy{ tPolicy };
	};

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolLockFree <T, tPolicy>::ObjectPoolLockFree()
	{
		mHead.Node = nullptr;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolLockFree <T, tPolicy>::ObjectPoolLockFree(size_t const initialCapacity)
	{
		mHead.Node = nullptr;

		Init(initialCapacity);
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPoolLockFree <T, tPolicy>::~ObjectPoolLockFree()
	{
		Clear(true);
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	void ObjectPoolLockFree <T, tPolicy>::Init(size_t const initialCapacity)
	{
		Clear(false);
		if (initialCapacity == 0)
		{
			return;
		}
		/*auto const [newAllocNode, count] = AllocateFreeBlockNodes(1);
		assert(newAllocNode != nullptr);

		mHead.Node = newAllocNode;
		mMaxCapacity = count;*/

		mHead.Node = nullptr;
		mMaxCapacity = 0;
		mUsedSize = 0;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	void ObjectPoolLockFree <T, tPolicy>::Clear(bool const isForce)
	{
		if (!isForce)
		{
			if (mUsedSize != 0)
			{
				throw std::logic_error("Return pool's objects before destroy");
			}
		}

		BlockNode* willRemoveNode = mHead.Node;

#if _DEBUG
		uint64_t capacityAcc = 0;
#endif
		while (willRemoveNode != nullptr)
		{
			BlockNode* const next = willRemoveNode->Next;

			assert(willRemoveNode->SafeUpper == LOCKFREE_BLOCKNODE_GUARD);
			assert(willRemoveNode->SafeLower == LOCKFREE_BLOCKNODE_GUARD);

			if constexpr (tPolicy == memory::eAllocationPolicy::None)
			{
				(&willRemoveNode->t)->~T();
			}
			free(willRemoveNode);

			willRemoveNode = next;

#if _DEBUG
			capacityAcc++;
#endif
		}

#if _DEBUG
		assert(capacityAcc <= mMaxCapacity);
#endif
		mHead.Node = nullptr;
		mMaxCapacity = 0;
		mUsedSize = 0;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	std::tuple<typename ObjectPoolLockFree<T, tPolicy>::BlockNode* const, size_t const>
		ObjectPoolLockFree<T, tPolicy>::AllocateFreeBlockNodes(size_t const blockCount) const
	{
		assert(blockCount == 1);
		assert(blockCount >= 0 && "blockCount should be positive");
		if (blockCount == 0)
		{
			return { nullptr, 0 };
		}

		//BlockNode* nextBlockNode = nullptr;

		//// NOTE(pyoung): malloc를 for문 밖에서 initialCapacity * size 로 할까?
		//for (size_t i = 0; i < blockCount; ++i)
		//{
		//	BlockNode* const currBlockNode = reinterpret_cast<BlockNode*>(malloc(sizeof(BlockNode)));
		//	if (currBlockNode == nullptr)
		//	{
		//		throw std::bad_alloc();
		//	}

		//	currBlockNode->SafeUpper = LOCKFREE_BLOCKNODE_GUARD;
		//	currBlockNode->SafeLower = LOCKFREE_BLOCKNODE_GUARD;
		//	currBlockNode->Next = nextBlockNode;
		//	if constexpr (tPolicy == memory::eAllocationPolicy::None)
		//	{
		//		new (&currBlockNode->t) T();
		//	}
		//	nextBlockNode = currBlockNode;
		//}

		//return { nextBlockNode, blockCount };

		BlockNode* const currBlockNode = reinterpret_cast<BlockNode*>(malloc(sizeof(BlockNode)));
		if (currBlockNode == nullptr)
		{
			throw std::bad_alloc();
		}

		currBlockNode->SafeUpper = LOCKFREE_BLOCKNODE_GUARD;
		currBlockNode->SafeLower = LOCKFREE_BLOCKNODE_GUARD;
		currBlockNode->Next = nullptr;
		if constexpr (tPolicy == memory::eAllocationPolicy::None)
		{
			new (&currBlockNode->t) T();
		}
		return { currBlockNode, blockCount };
	}

	template<typename T, memory::eAllocationPolicy tPolicy>
	template<typename... Args>
	T* ObjectPoolLockFree <T, tPolicy>::Rent(Args&&... args)
	{
		BlockNode* allocBlockNode = nullptr;
		if (InterlockedDecrement64(&mCachedSize) < 0)
		{
			InterlockedIncrement64(&mCachedSize);
			auto [newAllocNode, count] = AllocateFreeBlockNodes(1);
			if (count == 0)
			{
				throw std::bad_alloc();
			}

			allocBlockNode = newAllocNode;
			InterlockedIncrement(&mMaxCapacity);
		}
		else
		{
			alignas(16) Indexer headSnapshot;
			headSnapshot.Node = mHead.Node;
			headSnapshot.UID = mHead.UID;
			assert(headSnapshot.Node != nullptr);

			while (true)
			{
				// NOTE(pyoung): https://docs.microsoft.com/en-us/windows/win32/api/winnt/nf-winnt-interlockedcompareexchange128
				if (!InterlockedCompareExchange128(
					reinterpret_cast<volatile int64_t*>(&mHead),
					reinterpret_cast<int64_t>(headSnapshot.Node->Next),
					headSnapshot.UID + 1,
					reinterpret_cast<int64_t*>(&headSnapshot)))
				{
					continue;
				}

				allocBlockNode = headSnapshot.Node;

				break;
			}
		}

		assert(allocBlockNode != nullptr);
		assert(allocBlockNode->SafeUpper == LOCKFREE_BLOCKNODE_GUARD);
		assert(allocBlockNode->SafeLower == LOCKFREE_BLOCKNODE_GUARD);

		T* const allocPtrT = &(allocBlockNode->t);
		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			new (allocPtrT) T(std::forward<Args>(args)...);
		}

		InterlockedIncrement64(&mUsedSize);

		return allocPtrT;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	bool ObjectPoolLockFree <T, tPolicy>::Return(T* const ptrT)
	{
		assert(ptrT != nullptr);

		if (ptrT == nullptr)
		{
			return false;
		}

		BlockNode* const curNode = GetPtrBlockNode(ptrT);
		assert(curNode != nullptr);
		assert(curNode->SafeUpper == LOCKFREE_BLOCKNODE_GUARD);
		assert(curNode->SafeLower == LOCKFREE_BLOCKNODE_GUARD);

		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			ptrT->~T();
		}

		alignas(16) Indexer headSnapshot;
		headSnapshot.Node = mHead.Node;
		headSnapshot.UID = mHead.UID;

		while (true)
		{
			curNode->Next = headSnapshot.Node;

			if (!InterlockedCompareExchange128(
				reinterpret_cast<volatile int64_t*>(&mHead),
				reinterpret_cast<int64_t>(curNode),
				headSnapshot.UID + 1,
				reinterpret_cast<int64_t*>(&headSnapshot)))
			{
				continue;
			}
			
			InterlockedIncrement64(&mCachedSize);
			InterlockedDecrement64(&mUsedSize);

			return true;
		}
	}
} //namespace nf::objectpool
