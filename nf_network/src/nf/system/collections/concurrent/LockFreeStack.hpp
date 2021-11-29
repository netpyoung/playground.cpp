#pragma once

#include <cstdint>
#include <cassert>
#include <malloc.h>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include "nf/objectpool/ObjectPoolLockFree.hpp"

namespace nf::system::collections::concurrent
{
	template <typename T>
	class LockFreeStack final
	{
	public:
		explicit LockFreeStack();
		explicit LockFreeStack(size_t const maxCapacity);
		virtual ~LockFreeStack();
		explicit LockFreeStack(LockFreeStack const& stack) = delete;
		LockFreeStack& operator=(LockFreeStack const&) = delete;
	public:
		bool Push(T const& inputData);
		bool TryPop(T* const outputData);
		void Init(size_t const maxCapacity);
		void Clear();
	public:
		inline size_t GetMaxCapacity() const
		{
			return mMaxCapacity;
		}

		inline size_t GetUsedSize() const
		{
			return mUsedSize;
		}

		inline bool IsEmpty() const
		{
			return (mHead.Node == nullptr);
		}
	private:

		struct Node
		{
			Node* Next;
			T Data;
		};

#pragma pack(push, 1)
		struct alignas(16) Indexer
		{
			uint64_t UID{ 0 };
			Node* Node{ nullptr };
		};
#pragma pack(pop)

	private:
		alignas(16) Indexer mHead;
		objectpool::ObjectPoolLockFree<Node, memory::eAllocationPolicy::None> mPool;
		int64_t mUsedSize{ 0 };
		size_t mMaxCapacity{ 0 };
	};

	template <typename T>
	LockFreeStack<T>::LockFreeStack()
	{
		mHead.Node = nullptr;
	}

	template <typename T>
	LockFreeStack<T>::LockFreeStack(size_t const maxCapacity)
	{
		mHead.Node = nullptr;
		Init(maxCapacity);
	}

	template <typename T>
	LockFreeStack<T>::~LockFreeStack()
	{
		Clear();
	}

	template <typename T>
	void LockFreeStack<T>::Init(size_t const maxCapacity)
	{
		Clear();
		mPool.Init(maxCapacity);
		mMaxCapacity = maxCapacity;
		mHead.UID = 0;
		mHead.Node = nullptr;
	}

	template <typename T>
	void LockFreeStack<T>::Clear()
	{
		Node* node;
		while (mHead.Node != nullptr)
		{
			node = mHead.Node;
			mHead.Node = mHead.Node->Next;
			mPool.Return(node);
		}
		mMaxCapacity = 0;
		mPool.Clear(false);
	}

	template <typename T>
	bool LockFreeStack<T>::Push(T const& inputData)
	{
		if (static_cast<size_t>(mUsedSize) > mMaxCapacity)
		{
			return false;
		}

		Node* const newNode = mPool.Rent();
		newNode->Data = inputData;

		alignas(16) Indexer snapshotHead;
		snapshotHead.UID = mHead.UID;
		snapshotHead.Node = mHead.Node;

		while (true)
		{
			newNode->Next = snapshotHead.Node;

			if (!InterlockedCompareExchange128(
				reinterpret_cast<volatile int64_t*>(&mHead),
				reinterpret_cast<int64_t>(newNode),
				snapshotHead.UID + 1,
				reinterpret_cast<int64_t*>(&snapshotHead)))
			{
				continue;
			}

			InterlockedIncrement64(&mUsedSize);
			return true;
		}
	}

	template <typename T>
	bool LockFreeStack<T>::TryPop(T* const outputData)
	{
		if (InterlockedDecrement64(&mUsedSize) < 0)
		{
			InterlockedIncrement64(&mUsedSize);
			return false;
		}

		alignas(16) Indexer snapshotHead;
		snapshotHead.UID = mHead.UID;
		snapshotHead.Node = mHead.Node;
		assert(snapshotHead.Node != nullptr);

		while (true)
		{
			if (!InterlockedCompareExchange128(
				reinterpret_cast<volatile int64_t*>(&mHead),
				reinterpret_cast<int64_t>(snapshotHead.Node->Next),
				snapshotHead.UID + 1,
				reinterpret_cast<int64_t*>(&snapshotHead)))
			{
				continue;
			}

			*outputData = snapshotHead.Node->Data;
			mPool.Return(snapshotHead.Node);

			return true;
		}
	}
} // namespace nf::system::collections::concurrent