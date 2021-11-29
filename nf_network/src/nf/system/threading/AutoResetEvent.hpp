#pragma once

#include "WaitHandle.hpp"

namespace nf::system::threading
{
	class AutoResetEvent final : public WaitHandle
	{
	public:
		AutoResetEvent(bool const initialState) : WaitHandle(CreateEvent(nullptr, false, initialState, nullptr))
		{
			// CreateEvent(
			// _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
			// _In_ BOOL bManualReset,
			// _In_ BOOL bInitialState,
			// _In_opt_ LPCSTR lpName
			// )
		}

		AutoResetEvent(AutoResetEvent const& other) = delete;
		AutoResetEvent& operator=(AutoResetEvent const& other) = delete;
		virtual ~AutoResetEvent() = default;
	};
} // namespace nf::system::threading
