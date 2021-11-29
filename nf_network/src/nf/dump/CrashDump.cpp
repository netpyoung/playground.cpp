#include "CrashDump.hpp"

namespace nf::dump
{
	static uint64_t mDumpCount = 0;
#pragma warning(push, 0)
#pragma warning(disable: 4505)
	static void IvalidParameterHandler(
		wchar_t const* /* expression */,
		wchar_t const* /* function */,
		wchar_t const* /* file */,
		uint32_t /* line */,
		uintptr_t /* pReserved */)
	{
		CrashDump::Crash();
	}
	static int32_t CustomReportHookHandler(int32_t /* reportType */, char* /* message */, int32_t* /* returnValue */)
	{
		CrashDump::Crash();
		return 1;
	}

	static void CustomPureCallHandler()
	{
		CrashDump::Crash();
	}
#pragma warning(pop)

	static LONG WINAPI MyUnhandledExceptionFilter(_In_  _EXCEPTION_POINTERS* exceptionInfo)
	{
		uint64_t const dumpCount = ::InterlockedIncrement(&mDumpCount);
		::HANDLE const currentProcessHandle = ::GetCurrentProcess();
		if (currentProcessHandle == nullptr)
		{
			return 0;
		}

		// ref: https://docs.microsoft.com/en-us/windows/win32/api/psapi/nf-psapi-getprocessmemoryinfo
		PROCESS_MEMORY_COUNTERS pmc;
		int32_t workingMemorySize = 0;
		if (GetProcessMemoryInfo(currentProcessHandle, &pmc, sizeof(pmc)))
		{
			workingMemorySize = static_cast<int32_t>(pmc.WorkingSetSize / 1024 / 1024);
		}

		char filename[MAX_PATH];
		SYSTEMTIME localTime;
		::GetLocalTime(&localTime);
		wsprintf(filename, "Dump_%d%02d%02d_%02d.%02d.%02d_%llu_%dMB.dmp",
			localTime.wYear, localTime.wMonth, localTime.wDay,
			localTime.wHour, localTime.wMinute,
			localTime.wSecond,
			dumpCount,
			workingMemorySize
			);
		printf("\n\n\n!! Crash Error !!! %d.%d.%d / %d:%d:%d \n",
			localTime.wYear, localTime.wMonth, localTime.wDay,
			localTime.wHour, localTime.wMinute,
			localTime.wSecond
			);
		printf("Now Save dump file ...\n");

		{
			::HANDLE const dumpFileHandle = ::CreateFile(
				filename,
				GENERIC_WRITE,
				FILE_SHARE_WRITE,
				NULL,
				CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL
				);
			if (dumpFileHandle == INVALID_HANDLE_VALUE)
			{
				return EXCEPTION_EXECUTE_HANDLER;
			}

			::_MINIDUMP_EXCEPTION_INFORMATION minidumpExceptionInformation;
			minidumpExceptionInformation.ThreadId = ::GetCurrentThreadId();
			minidumpExceptionInformation.ExceptionPointers = exceptionInfo;
			minidumpExceptionInformation.ClientPointers = TRUE;
			::MiniDumpWriteDump(
				currentProcessHandle,
				::GetCurrentProcessId(),
				dumpFileHandle,
				::MiniDumpWithFullMemory,
				&minidumpExceptionInformation,
				nullptr,
				nullptr
				);
			::CloseHandle(dumpFileHandle);
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	inline static void SetHandlerDump()
	{
		// ref: https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter
		::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);
	}

	CrashDump::CrashDump()
	{
//#ifndef _DEBUG
		mDumpCount = 0;
		::_invalid_parameter_handler const newHandler = IvalidParameterHandler;
		::_set_invalid_parameter_handler(newHandler);
		_CrtSetReportMode(_CRT_WARN, 0);
		_CrtSetReportMode(_CRT_ASSERT, 0);
		_CrtSetReportMode(_CRT_ERROR, 0);
		_CrtSetReportHook(CustomReportHookHandler);
		::_set_purecall_handler(CustomPureCallHandler);
		SetHandlerDump();
//#endif // !_DEBUG
	}

} // namespace nf::dump