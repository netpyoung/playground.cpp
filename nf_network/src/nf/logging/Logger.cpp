#include "Logger.hpp"

// TODO(pyoung): https://en.cppreference.com/w/cpp/language/variadic_arguments
// TODO(pyoung): https://docs.microsoft.com/en-us/windows/win32/api/strsafe/nf-strsafe-stringcchprintfa

namespace nf::logging
{
	Logger::Logger(eLogLevel const logLevel) :
		Logger(logLevel, std::cout)
	{
	}

	Logger::Logger(eLogLevel const logLevel, std::filesystem::path const path) :
		mLogLevel(logLevel),
		mFilePath(path)
	{
		::InitializeSRWLock(&mLock);
		//::InitializeCriticalSection(&mCriticalSection);
	}

	Logger::Logger(eLogLevel const logLevel, std::ostream& os) :
		mLogLevel(logLevel)
	{
		//::InitializeCriticalSection(&mCriticalSection);
		::InitializeSRWLock(&mLock);
		// ref: https://stdcxx.apache.org/doc/stdlibug/34-2.html
		mOstream.copyfmt(os); // Copy the values os member variables (other than the streambuffer and the iostate) in cout to out.
		mOstream.clear(os.rdstate());// Set state flags for out to the current state os std::cout.
		mOstream.basic_ios<char>::rdbuf(os.rdbuf()); // Replace out's streambuffer with std::cout's streambuffer.
	}

	Logger::~Logger()
	{
		//::DeleteCriticalSection(&mCriticalSection);
	}

	void Logger::SetFilePath(std::filesystem::path const path)
	{
		mFilePath = path;
	}

	void Logger::Trace(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::Trace, fmt, args);
		va_end(args);
	}

	void Logger::Debug(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::Debug, fmt, args);
		va_end(args);
	}

	void Logger::Info(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::Info, fmt, args);
		va_end(args);
	}

	void Logger::Warn(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::Warn, fmt, args);
		va_end(args);
	}

	void Logger::Error(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::Error, fmt, args);
		va_end(args);
	}

	void Logger::System(char const* const tag, char const* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		Log(tag, eLogLevel::System, fmt, args);
		va_end(args);
	}


	std::string Format(char const* const fmt, va_list const args)
	{
		if (fmt == nullptr)
		{
			return "";
		}

		std::unique_ptr<char[]> buffer;
		int32_t result = -1;
		size_t length = 512;
		while (result == -1)
		{
			buffer = std::make_unique<char[]>(length + 1);
			memset(buffer.get(), 0, length + 1);
			//result = vsnprintf(buffer.get(), length, fmt, args); // <stdio.h>
			result = ::StringCchVPrintf(buffer.get(), length, fmt, args);  // <strsafe.h>
			length *= 2;
		}
		return std::string(buffer.get());
	}

	char const* GetLogName(eLogLevel const logLevel)
	{
		switch (logLevel)
		{
		case eLogLevel::Trace:	return "[Trace ]";
		case eLogLevel::Debug:	return "[Debug ]";
		case eLogLevel::Info:	return "[Info  ]";
		case eLogLevel::Warn:	return "[Warn  ]";
		case eLogLevel::Error:	return "[Error ]";
		case eLogLevel::System:	return "[System]";
		default:				return "        ";
		}
	}

	typedef std::chrono::duration<int, std::ratio_multiply<std::chrono::hours::period, std::ratio<24>>::type> days;

	struct UtcTime
	{
		int32_t Year;
		int32_t Month;
		int32_t Day;
		int32_t Hour;
		int32_t Minute;
		int32_t Second;
		int32_t Millisecond;
	};

	void GetUtcTime(UtcTime* const utcTime)
	{
		std::chrono::time_point const now = std::chrono::system_clock::now();
		//uint64_t milliseconds_since_epoch = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>
		//	(now.time_since_epoch()).count());
		std::chrono::system_clock::duration tp = now.time_since_epoch();
		days d = std::chrono::duration_cast<days>(tp);
		tp -= d;
		std::chrono::hours h = std::chrono::duration_cast<std::chrono::hours>(tp);
		tp -= h;
		std::chrono::minutes m = std::chrono::duration_cast<std::chrono::minutes>(tp);
		tp -= m;
		std::chrono::seconds s = std::chrono::duration_cast<std::chrono::seconds>(tp);
		tp -= s;

		time_t tt = std::chrono::system_clock::to_time_t(now);
#pragma warning(push)
#pragma warning(disable: 4996)
		tm utc_tm = *gmtime(&tt);
#pragma warning(pop)

		utcTime->Year = utc_tm.tm_year + 1900;
		utcTime->Month = utc_tm.tm_mon + 1;
		utcTime->Day = utc_tm.tm_mday;
		utcTime->Hour = static_cast<int32_t>(h.count());
		utcTime->Minute = static_cast<int32_t>(m.count());
		utcTime->Second = static_cast<int32_t>(s.count());
		utcTime->Millisecond = static_cast<int32_t>(tp.count());
	}

	void Logger::Log(char const* const tag, eLogLevel const logLevel, char const* const fmt, va_list lst) noexcept
	{
		if (static_cast<int32_t>(logLevel) < static_cast<int32_t>(mLogLevel))
		{
			return;
		}
		std::stringstream stream = BaseInfo(tag, logLevel);
		stream << Format(fmt, lst);
		ForwardStream(stream);
	}

	void Logger::Log(char const* const tag, eLogLevel const logLevel, std::stringstream& ss) noexcept
	{
		if (static_cast<int32_t>(logLevel) < static_cast<int32_t>(mLogLevel))
		{
			return;
		}
		std::stringstream stream = BaseInfo(tag, logLevel);
		stream << ss.str();
		ForwardStream(stream);
	}

	void Logger::ForwardStream(std::stringstream& ss)
	{
		AcquireSRWLockExclusive(&mLock);
		//::EnterCriticalSection(&mCriticalSection);

		if (!mFilePath.empty())
		{
			std::ofstream ofs;
			ofs.open(mFilePath, std::fstream::out | std::fstream::app);
			ofs << ss.str() << std::endl << std::flush;
			ofs.flush();
			ofs.close();
		}
		else
		{
			mOstream << ss.str() << std::endl << std::flush;
		}
		//::LeaveCriticalSection(&mCriticalSection);
		ReleaseSRWLockExclusive(&mLock);
	}

	std::stringstream Logger::BaseInfo(char const* const tag, eLogLevel const logLevel) noexcept
	{
		std::stringstream ss;
		std::thread::id const threadId = std::this_thread::get_id();

		int64_t const seqNumber = InterlockedIncrement64(&mSequenceNumber);
		UtcTime utcTime;
		GetUtcTime(&utcTime);
		ss << GetLogName(logLevel) << "|" << tag << "|"
			<< utcTime.Year << "." << utcTime.Month << "." << utcTime.Day << "|"
			<< utcTime.Hour << utcTime.Minute << utcTime.Second << "|"
			<< "<" << threadId << ">|"
			<< seqNumber << "|";
		return ss;
	}

	void Logger::LogHex(char const* const tag, eLogLevel const logLevel, std::byte const* const bytes, size_t const size) noexcept
	{
		// ref: https://stackoverflow.com/questions/5760252/how-can-i-print-0x0a-instead-os-0xa-using-cout/35963334
		// C++20
		// #include <fmt/format.h>
		// std::cout << fmt::format("{:#04x}\n", 10);

		std::stringstream ss = BaseInfo(tag, logLevel);
		ss << std::hex << std::internal;
		for (size_t i = 0; i < size; ++i)
		{
			if (i % 8 == 0)
			{
				ss << std::endl;
			}
			ss << "0x" << std::setw(2) << std::setfill('0') << static_cast<int32_t>(bytes[i]) << ' ';
		}
		ForwardStream(ss);
	}
} // namespace nf::logging