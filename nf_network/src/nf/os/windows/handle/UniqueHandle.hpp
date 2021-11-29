#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

namespace nf::os::windows::handle
{
	template <typename T>
	class UniqueHandle
	{
	public:
		explicit UniqueHandle(HANDLE const value = T::Invalid()) :
			mHandle{ value }
		{
		}

		UniqueHandle(UniqueHandle&& other) :
			mHandle{ other.Release() }
		{
		}
		UniqueHandle(UniqueHandle const&) = delete;

		~UniqueHandle()
		{
			Close();
		}

		UniqueHandle& operator=(UniqueHandle const&) = delete;

		UniqueHandle& operator=(UniqueHandle&& other) noexcept
		{
			if (this != &other)
			{
				Reset(other.Release());
			}

			return *this;
		}

		explicit operator bool() const
		{
			return mHandle != T::Invalid();
		}

		HANDLE Release()
		{
			HANDLE const value = mHandle;
			mHandle = T::Invalid();
			return value;
		}

		bool Reset(HANDLE const value = T::Invalid())
		{
			if (mHandle != value)
			{
				Close();
				mHandle = value;
			}

			return static_cast<bool>(*this);
		}
	public:
		inline HANDLE Get() const
		{
			return mHandle;
		}

	private:
		inline void Close() const
		{
			if (*this)
			{
				T::Exit(mHandle);
			}
		}
	private:
		HANDLE mHandle;
	};
} // namespace nf::os::windows::handle

namespace nf::os::windows::handle
{
	struct NullHandleBehaviour
	{
		static HANDLE Invalid()
		{
			return nullptr;
		}

		static void Exit(HANDLE const val)
		{
			CloseHandle(val);
		}
	};

	using NullHandle = UniqueHandle<NullHandleBehaviour>;
} // namespace nf::os::windows::handle
