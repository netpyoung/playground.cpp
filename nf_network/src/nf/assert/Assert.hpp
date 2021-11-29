#pragma once

#include "nf/dump/CrashDump.hpp"
#include <cassert>
namespace nf::assert
{

#ifdef _DEBUG
	constexpr bool debug_mode = true;
#else
	constexpr bool debug_mode = false;
#endif

	class Assertion
	{
	public:
		Assertion() = default;
		virtual ~Assertion() = default;
	public:
		static constexpr bool True(bool const assertion, char const* const /*msg*/ = nullptr)
		{
			if constexpr (debug_mode)
			{
				assert(assertion);
			}
			else
			{
				if (!assertion)
				{
					mDump.Crash();
				}
			}
			return assertion;
		}
	private:
		static dump::CrashDump mDump;
	};
} // namespace nf::assert