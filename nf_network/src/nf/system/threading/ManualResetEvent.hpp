#pragma once

#include "WaitHandle.hpp"

namespace nf::system::threading
{
	class ManualResetEvent final : public WaitHandle
	{
	public:
		ManualResetEvent(bool const initialState = false) : WaitHandle(CreateEvent(nullptr, true, initialState, nullptr))
		{
			// CreateEvent(
			// _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
			// _In_ BOOL bManualReset,
			// _In_ BOOL bInitialState,
			// _In_opt_ LPCSTR lpName
			// )
		}

		ManualResetEvent(ManualResetEvent const& other) = delete;
		ManualResetEvent& operator=(ManualResetEvent const& other) = delete;
		virtual ~ManualResetEvent() = default;
	};
} // namespace nf::system::threading
