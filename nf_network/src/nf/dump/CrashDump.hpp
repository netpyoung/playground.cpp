#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Crtdbg.h>
#include <cstdlib>
#include <errhandlingapi.h>
#include <cstdint>
#include <WinBase.h>
#include <stdio.h>
#pragma comment(lib, "Psapi.lib")
#include <Psapi.h>

#pragma comment(lib, "DbgHelp.Lib")
#include <DbgHelp.h>

namespace nf::dump
{
	class CrashDump
	{
	public:
		CrashDump();
		virtual ~CrashDump() = default;

		inline static void Crash()
		{
			int32_t* p = nullptr;
#pragma warning(push, 0)
#pragma warning(disable: 6011)
			* p = 0;
#pragma warning(pop)
		}
	};
} // namespace nf::dump