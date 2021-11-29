#pragma once

#include <cstdint>
#include <cassert>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <memory>
#include "nf/objectpool/ObjectPoolLockFree.hpp"

// ref: https://www.slideshare.net/kumagi/lockfree-queue-3520350

// TODO(pyoung): https://www.youtube.com/watch?v=mu6XB-WRNxs
// C++Now 2018: Tony Van Eerd “The Continuing Saga of the Lock-free Queue: Part 3 of N”

// ref: https://github.com/boostcon/cppnow_presentations_2018/blob/master/05-08-2018_tuesday/the_continuing_saga_of_the_lockfree_queue__tony_van_eerd__cppnow_05082018.pdf

namespace nf::system::collections::concurrent
{
	template <typename T>
	class LockFreeQueue final
	{
	public:
		explicit LockFreeQueue(size_t const maxCapacity);
		virtual ~LockFreeQueue();
		explicit LockFreeQueue(LockFreeQueue const& stack) = delete;
		LockFreeQueue& operator=(LockFreeQueue const&) = delete;
	public:
		bool Enqueue(T const& t);
		bool TryDequeue(T* const out);
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
			return (mUsedSize <= 0);
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
			Node* DummyNode{ nullptr };
		};
#pragma pack(pop)

	private:
		alignas(16) Indexer mHead;
		alignas(16) Indexer mTail;

		objectpool::ObjectPoolLockFree<Node, memory::eAllocationPolicy::None> mPool;
		
		int64_t mUsedSize{ 0 };
		size_t mMaxCapacity{ 0 };
	};

	template <typename T>
	LockFreeQueue<T>::LockFreeQueue(size_t const maxCapacity)
	{
		mHead.DummyNode = nullptr;
		Init(maxCapacity);
	}

	template <typename T>
	void LockFreeQueue<T>::Init(size_t const maxCapacity)
	{
		mPool.Init(maxCapacity);
		Clear();
		mMaxCapacity = maxCapacity;
	}

	template <typename T>
	LockFreeQueue<T>::~LockFreeQueue()
	{
		Clear();
		mPool.Return(mHead.DummyNode);
		mHead.DummyNode = nullptr;
		mTail.DummyNode = nullptr;
	}

	template <typename T>
	void LockFreeQueue<T>::Clear()
	{
		if (mHead.DummyNode != nullptr)
		{
			Node* freeNode = mHead.DummyNode->Next;
			while (freeNode != nullptr)
			{
				mPool.Return(freeNode);
				freeNode = freeNode->Next;
			}
			mPool.Return(mHead.DummyNode);
			mHead.DummyNode = nullptr;
		}
		
		Node* const dummyNode = mPool.Rent();
		dummyNode->Next = nullptr;

		mHead.UID = 0;
		mHead.DummyNode = dummyNode;

		mTail.UID = 0;
		mTail.DummyNode = dummyNode;
	}

	template <typename T>
	bool LockFreeQueue<T>::Enqueue(T const& inputData)
	{
		alignas(16) Node* const newNode = mPool.Rent();
		newNode->Data = inputData;
		newNode->Next = nullptr;

		alignas(16) Indexer snapshotTail;
		while (true)
		{
			snapshotTail.DummyNode = mTail.DummyNode;
			snapshotTail.UID = mTail.UID;

			Node const* const snapshotTailNodeNext = snapshotTail.DummyNode->Next;

			// (1) 누가 먼저 Enqueue해버린 것. - Tail이 잘 안 옮겨진것 같으니 옮겨주자
			if (snapshotTailNodeNext != nullptr)
			{
				InterlockedCompareExchange128(
					reinterpret_cast<volatile int64_t*>(&mTail),
					reinterpret_cast<int64_t>(snapshotTailNodeNext),
					snapshotTail.UID + 1,
					reinterpret_cast<int64_t*>(&snapshotTail));
				continue;
			}

			// (2) 새로운 Tail 연결 시도.
			if (InterlockedCompareExchangePointer(
				reinterpret_cast<volatile PVOID*>(&mTail.DummyNode->Next),
				newNode,
				nullptr) != nullptr)
			{
				continue;
			}

			// (3) Tail 이동.
			InterlockedCompareExchange128(
				reinterpret_cast<volatile int64_t*>(&mTail),
				reinterpret_cast<int64_t>(newNode),
				snapshotTail.UID + 1,
				reinterpret_cast<int64_t*>(&snapshotTail));

			// 어쨋든 새로운노드 연결시켰으니 크기 증가.
			InterlockedIncrement64(&mUsedSize);

			return true;
		}
	}

	template <typename T>
	bool LockFreeQueue<T>::TryDequeue(T* const outputData)
	{
		if (InterlockedDecrement64(&mUsedSize) < 0)
		{
			InterlockedIncrement64(&mUsedSize);
			return false;
		}

		alignas(16) Indexer snapshotHead;
		alignas(16) Indexer snapshotTail;
		while (true)
		{
			snapshotHead.DummyNode = mHead.DummyNode;
			snapshotHead.UID = mHead.UID;

			snapshotTail.DummyNode = mTail.DummyNode;
			snapshotTail.UID = mTail.UID;

			Node const* const nextDummyNode = snapshotHead.DummyNode->Next;
			Node const* const snapshotTailNodeNext = snapshotTail.DummyNode->Next;

			// (1) 그사이 누군가 먼저 Dequeue해 버려, 아직 Dequeue할 것이 없는 상태.
			if (nextDummyNode == nullptr)
			{
				YieldProcessor();
				continue;
			}

			// (2) Tail이 충분히 밀려지지 않은 상태.
			if (snapshotTailNodeNext != nullptr)
			{
				// (2-1) 밀어보고
				if (!InterlockedCompareExchange128(
					reinterpret_cast<volatile int64_t*>(&mTail),
					reinterpret_cast<int64_t>(snapshotTailNodeNext),
					snapshotTail.UID + 1,
					reinterpret_cast<int64_t*>(&snapshotTail)))
				{
					continue;
				}

				// (2-2) 끝까지 밀어졌는지 확인.
				if (mTail.DummyNode->Next != nullptr)
				{
					continue;
				}
			}

			// 데이터를 확보하고..
			//T const& snapshotData = nextDummyNode->Data; // NOTE(pyoung): 이렇게 레퍼런스로 받으면 `버그`남.
			*outputData = nextDummyNode->Data;

			// (3) Head를 교체 시도.
			if (!InterlockedCompareExchange128(
				reinterpret_cast<volatile int64_t*>(&mHead),
				reinterpret_cast<int64_t>(nextDummyNode),
				snapshotHead.UID + 1,
				reinterpret_cast<int64_t*>(&snapshotHead)))
			{
				continue;
			}

			// 메모리풀 해제.
			mPool.Return(snapshotHead.DummyNode);
			return true;
		}
	}
} // namespace nf::system::collections::concurrent