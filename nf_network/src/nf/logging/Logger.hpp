#pragma once

#pragma warning(push, 0)
#include <cstdarg>
#include <cstdint>
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <Windows.h>
#include <strsafe.h>
#include <chrono>
#include <string.h>
#include <cstdio>
#include <iostream>
#include <ctime>
#include <thread>
#include <functional>
#include <cassert>
#include <iomanip>     // For setfill, setw.
#include <sstream>
#include <filesystem>
#include <fstream>
#pragma warning(pop)


#include "eLogLevel.hpp"
#include "LogStreamBuf.hpp"

// TODO(pyoung): https://github.com/gabime/spdlog

// c++20: https://en.cppreference.com/w/cpp/chrono/utc_clock/now
namespace nf::logging
{

#pragma warning(push)
#pragma warning (disable: 4820)
	class Logger
	{
	public:
		Logger(eLogLevel const logLevel);
		Logger(eLogLevel const logLevel, std::filesystem::path const path);
		Logger(eLogLevel const logLevel, std::ostream& os);
		~Logger();
	public:
		Logger(Logger const& logger) = delete;
		Logger& operator=(Logger const& logger) = delete;
	public:
		void SetFilePath(std::filesystem::path const path);
		void Log(char const* const tag, eLogLevel const logLevel, char const* const fmt, va_list lst) noexcept;
		// TODO(pyoung)
		// Type_년월.log
		// [Battle][Debug][2020.09.02/19:00:00][seqnumber] logtext...
		// Type | Level | Fmt | Param1, ...
		void Trace(char const* const tag, char const* const fmt, ...) noexcept;
		void Debug(char const* const tag, char const* const fmt, ...) noexcept;
		void Info(char const* const tag, char const* const fmt, ...) noexcept;
		void Warn(char const* const tag, char const* const fmt, ...) noexcept;
		void Error(char const* const tag, char const* const fmt, ...) noexcept;
		void System(char const* const tag, char const* const fmt, ...) noexcept;
		// TOOD(pyoung) add Crash
		// Trace / Debug / Info / Warn / Error / Crash
	public:
		inline void SetLogLevel(eLogLevel const logLevel) noexcept
		{
			mLogLevel = logLevel;
		}
		inline eLogLevel GetLogLevel() const noexcept
		{
			return mLogLevel;
		}
	public:

		// TODO(pyoung)
		// https://github.com/google/glog/blob/a6f7be14c654b49f2907ffd1bce2aaad9571e579/src/windows/glog/logging.h
		//inline std::ostream& LOG(eLogLevel const severity)
		//{
		//	return mStream;
		//}
		//LogStreamBuf mLogStreamBuf;
		//std::ostream mStream{ &mLogStreamBuf };
		// D_INFO
		// D_ERROR
		//constexpr std::ostream& D_INFO(bool a)
		//{
		//	if constexpr (_DEBUG)
		//	{
		//		if (a)
		//		{
		//			return mStream;
		//		}
		//		return mStream;
		//	}
		//	else
		//	{
		//		return mStream;
		//	}
		//}
		//static std::ostream mStream;
	public:
		inline void DebugHex(char const* const tag, std::byte const* const bytes, size_t const size) noexcept
		{
			LogHex(tag, eLogLevel::Debug, bytes, size);
		}
		inline void InfoHex(char const* const tag, std::byte const* const bytes, size_t const size) noexcept
		{
			LogHex(tag, eLogLevel::Info, bytes, size);
		}
		inline void WarnHex(char const* const tag, std::byte const* const bytes, size_t const size) noexcept
		{
			LogHex(tag, eLogLevel::Warn, bytes, size);
		}
		inline void ErrorHex(char const* const tag, std::byte const* const bytes, size_t const size) noexcept
		{
			LogHex(tag, eLogLevel::Error, bytes, size);
		}
		inline void SystemHex(char const* const tag, std::byte const* const bytes, size_t const size) noexcept
		{
			LogHex(tag, eLogLevel::System, bytes, size);
		}
	protected:
		void Log(char const* const tag, eLogLevel const logLevel, std::stringstream& ss) noexcept;
		std::stringstream Logger::BaseInfo(char const* const tag, eLogLevel const logLevel) noexcept;
		void ForwardStream(std::stringstream& ss);
		void LogHex(char const* const tag, eLogLevel const logLevel, std::byte const* const bytes, size_t const size) noexcept;
	private:
		int64_t mSequenceNumber{ 0 };
		eLogLevel mLogLevel{ eLogLevel::Debug };
		std::filesystem::path mFilePath;
		std::ofstream mOstream;
		SRWLOCK mLock;
		CRITICAL_SECTION mCriticalSection;
	};
#pragma warning(pop)
} // namespace nf::logging


/*
#undef assert

#ifdef NDEBUG

#define assert(expression) ((void)0)

#else

_ACRTIMP void __cdecl _wassert(
	_In_z_ wchar_t const* _Message,
	_In_z_ wchar_t const* _File,
	_In_   unsigned       _Line
);

#define assert(expression) (void)(                                                       \
			(!!(expression)) ||                                                              \
			(_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \
		)

#endif
*/

/*
namespace nf::logging
{
	// NFLog(std::cout, debug, "hello", << a << "wtf" << "A");
	// NFLog(logger, debug, tag, << a << b << c);
#pragma warning(push)
#pragma warning (disable: 4505)
	static void __cdecl Hello(std::ostream& os)
	{
		os << std::endl;
	}
#pragma warning(pop)

#define NFLog(stream, level, tag, expression)(void) ( \
			(nf::logging::Hello(stream << #level << tag << __FILE__ << " " << __LINE__ << " " expression)) \
		)
	// #define NFLog
} // nf::logging
*/