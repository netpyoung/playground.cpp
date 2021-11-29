#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <array>
#pragma warning(pop)

#include <cstdint>
#include <cassert>
#include "nf/os/windows/handle/UniqueHandle.hpp"

namespace nf::system::threading
{
	class WaitHandle
	{
	public:
		WaitHandle(HANDLE handle) :
			mEventHandle(os::windows::handle::NullHandle(handle))
		{
			// _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
			// _In_ BOOL bManualReset,
			// _In_ BOOL bInitialState,
			// _In_opt_ LPCSTR lpName
			assert(mEventHandle);
		}

		WaitHandle(WaitHandle const& other) = delete;
		WaitHandle& operator=(WaitHandle const& other) = delete;
		virtual ~WaitHandle() = default;
	public:
		inline bool WaitOne(uint32_t const waitMs = INFINITE)
		{
			return WaitForSingleObject(mEventHandle.Get(), waitMs) == WAIT_OBJECT_0;
		}

		inline void Set()
		{
			SetEvent(mEventHandle.Get());
		}

		inline void Reset()
		{
			ResetEvent(mEventHandle.Get());
		}

		inline HANDLE GetHandle()
		{
			return mEventHandle.Get();
		}
	public:
		// =============================================
		// Static
		// =============================================
		template<size_t tSize>
		static std::pair<int32_t, int32_t> WaitAll(std::array<threading::WaitHandle*, tSize> const& events, uint32_t const waitMs = INFINITE)
		{
			return WaitForMultipleObjects(events, true, waitMs);
		}

		template<size_t tSize>
		static std::pair<int32_t, int32_t> WaitAny(std::array<threading::WaitHandle*, tSize> const& events, uint32_t const waitMs = INFINITE)
		{
			return WaitForMultipleObjects(events, false, waitMs);
		}
	private:
		template<size_t tSize>
		static std::pair<int32_t, int32_t> WaitForMultipleObjects(std::array<threading::WaitHandle*, tSize> const& events, bool const isAll, uint32_t const waitMs)
		{
			std::array<HANDLE, tSize> handles;
			for (size_t i = 0; i < tSize; ++i)
			{
				handles[i] = events[i]->GetHandle();
			}

			return _WaitForMultipleObjects(handles.data(), tSize, isAll, waitMs);
		}
		static std::pair<int32_t, int32_t> _WaitForMultipleObjects(HANDLE const*const handles, size_t size, bool const isAll, uint32_t const waitMs)
		{
			// ref: https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
			//assert(size <= MAXIMUM_WAIT_OBJECTS);
			uint32_t const result = ::WaitForMultipleObjects(static_cast<DWORD>(size), handles, isAll, waitMs);
			if (result == WAIT_FAILED)
			{
				return { -1, 0 };
			}
			if (result == WAIT_TIMEOUT)
			{
				return { -1, 0 };
			}
			if (result >= WAIT_ABANDONED_0)
			{
				return { -1, 0 };
			}
			//assert(WAIT_OBJECT_0 <= result);
			assert(result < WAIT_OBJECT_0 + size);
			return { 0,  static_cast<int32_t>(result) };
		}
	private:
		os::windows::handle::NullHandle mEventHandle;
	};
} // namespace nf::system::threading
