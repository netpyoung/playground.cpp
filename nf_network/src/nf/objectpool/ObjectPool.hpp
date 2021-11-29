#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <tuple>
#include "nf/memory/eAllocationPolicy.hpp"

namespace nf::objectpool
{
	template <typename T, memory::eAllocationPolicy tPolicy>
	class ObjectPool final
	{
	public:
		explicit ObjectPool(int32_t const initialCapacity);
		ObjectPool(ObjectPool& other) = delete;
		ObjectPool& operator=(ObjectPool const& other) = delete;
		virtual ~ObjectPool();

	public:
		template<typename... Args>
		T* Rent(Args&&... args);

		bool Return(T* const ptrT);
		void Clear();

	public:
		inline int32_t GetMaxCapacity() const
		{
			return mMaxCapacity;
		}

		inline int32_t GetUsedSize() const
		{
			return mUsedSize;
		}

	private:
		// [Next*][SafeUpper][t][SafeLower]
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

		std::tuple<BlockNode* const, int32_t const> AllocateFreeBlockNodes(int32_t const blockCount) const;

	private:
		inline ObjectPool<T, tPolicy>::BlockNode* GetPtrBlockNode(T* const ptrT) const
		{
			return reinterpret_cast<BlockNode*>(reinterpret_cast<std::byte*>(ptrT) - sizeof(BlockNode*) - sizeof(int64_t));
		}
	private:
		int64_t const GUARD = 0x1234567890ABCDEF;
		int32_t mMaxCapacity{ 0 };
		int32_t mUsedSize{ 0 };
		int32_t mCachedSize{ 0 };
		memory::eAllocationPolicy mPolicy{ tPolicy };
		BlockNode* mFreeBlockNodeHeader;
	};

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPool<T, tPolicy>::ObjectPool(int32_t const initialCapacity)
	{
		auto const [header, count] = AllocateFreeBlockNodes(initialCapacity);

		mFreeBlockNodeHeader = header;
		mMaxCapacity += count;
		mCachedSize += count;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	void ObjectPool<T, tPolicy>::Clear()
	{
		// TODO(pyoung): 검증다시
		while (mFreeBlockNodeHeader != nullptr)
		{
			BlockNode* const next = mFreeBlockNodeHeader->Next;

			assert(mFreeBlockNodeHeader->SafeUpper == GUARD);
			assert(mFreeBlockNodeHeader->SafeLower == GUARD);

			if constexpr (tPolicy == memory::eAllocationPolicy::None)
			{
				(&mFreeBlockNodeHeader->t)->~T();
			}
			free(mFreeBlockNodeHeader);

			mFreeBlockNodeHeader = next;
		}
		auto const [header, count] = AllocateFreeBlockNodes(1);

		mFreeBlockNodeHeader = header;
		mMaxCapacity += count;
		mCachedSize += count;
		mUsedSize = 0;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	ObjectPool<T, tPolicy>::~ObjectPool()
	{
		assert(mUsedSize == 0 && "Return pool's objects before destroy");

		BlockNode* next;

#if _DEBUG
		uint64_t capacityAcc = 0;
#endif

		while (mFreeBlockNodeHeader != nullptr)
		{
			next = mFreeBlockNodeHeader->Next;

			assert(mFreeBlockNodeHeader->SafeUpper == GUARD);
			assert(mFreeBlockNodeHeader->SafeLower == GUARD);

			if constexpr (tPolicy == memory::eAllocationPolicy::None)
			{
				(&mFreeBlockNodeHeader->t)->~T();
			}
			free(mFreeBlockNodeHeader);

			mFreeBlockNodeHeader = next;
#if _DEBUG
			capacityAcc++;
#endif
		}

#if _DEBUG
		assert(capacityAcc == mMaxCapacity);
#endif
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	std::tuple<typename ObjectPool<T, tPolicy>::BlockNode* const, int32_t const>
		ObjectPool<T, tPolicy>::AllocateFreeBlockNodes(int32_t const blockCount) const
	{
		assert(blockCount >= 0 && "blockCount should be positive");
		if (blockCount == 0)
		{
			return { nullptr, 0 };
		}

		BlockNode* prevBlockNode = nullptr;
		BlockNode* currBlockNode = nullptr;

		// NOTE(pyoung): malloc를 for문 밖에서 하는게 더 좋을까?
		for (int32_t i = 0; i < blockCount; ++i)
		{
			currBlockNode = reinterpret_cast<BlockNode*>(malloc(sizeof(BlockNode)));
			if (currBlockNode == nullptr)
			{
				throw std::runtime_error("fail to allocate of BlockNodes");
			}
			if constexpr (tPolicy == memory::eAllocationPolicy::None)
			{
				new (&currBlockNode->t) T();
			}
			currBlockNode->SafeUpper = GUARD;
			currBlockNode->SafeLower = GUARD;
			currBlockNode->Next = prevBlockNode;
			prevBlockNode = currBlockNode;
		}

		return { currBlockNode, blockCount };
	}

	template<typename T, memory::eAllocationPolicy tPolicy>
	template<typename... Args>
	T* ObjectPool<T, tPolicy>::Rent(Args&&... args)
	{
		if (mFreeBlockNodeHeader == nullptr)
		{
			auto [header, count] = AllocateFreeBlockNodes(1);
			mFreeBlockNodeHeader = header;
			mMaxCapacity += count;
		}

		T* const ptrT = &mFreeBlockNodeHeader->t;
		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			new (ptrT) T(std::forward<Args>(args)...);
		}

		{
			mFreeBlockNodeHeader = mFreeBlockNodeHeader->Next;
			mUsedSize++;
			mCachedSize--;
		}
		return ptrT;
	}

	template <typename T, memory::eAllocationPolicy tPolicy>
	bool ObjectPool<T, tPolicy>::Return(T* const ptrT)
	{
		BlockNode* const currNode = GetPtrBlockNode(ptrT);

		assert(currNode->SafeUpper == GUARD);
		assert(currNode->SafeLower == GUARD);

		if constexpr (tPolicy == memory::eAllocationPolicy::CallConstructorDestructor)
		{
			ptrT->~T();
		}

		{
			currNode->Next = mFreeBlockNodeHeader;
			mFreeBlockNodeHeader = currNode;
			mUsedSize--;
			mCachedSize++;
		}
		return true;
	}
} //namespace nf::objectpool