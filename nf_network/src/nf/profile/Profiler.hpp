#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <Windows.h>
#include <profileapi.h>
#pragma warning(pop)

#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <timeapi.h>
#include <array>
#include <stdexcept>
#include <thread>
#include <unordered_set>
#include <algorithm>

#include "eOption.hpp"
#include "ProfileSample.hpp"
#include "ProfileThreadGroup.hpp"

namespace nf::profile
{
	extern __declspec(thread) int64_t gTLSThreadIndex;

	constexpr size_t MAX_SUPPORT_THREAD = 8;

	template<eOption tOption>
	class Profiler final
	{
	public:
		Profiler();
		~Profiler() = default;
	public:
		constexpr void Begin(char const* const tag);
		constexpr void End(char const* const tag);
		constexpr void Dump();
	private:
		constexpr void Clear();
		void Dump(std::ostream& os);
		LARGE_INTEGER mFrequency{ 0 };
		int64_t mThreadIndexAcc{ -1 };
		int64_t mFileUIDAcc{ -1 };
		std::array<ProfileThreadGroup, MAX_SUPPORT_THREAD> mProfileGroups;
	};

	template<eOption tOption>
	Profiler<tOption>::Profiler()
	{
		if constexpr (tOption == eOption::Enable)
		{
			::timeBeginPeriod(1);
			bool const isSupport = ::QueryPerformanceFrequency(&mFrequency);
			if (!isSupport)
			{
				throw std::bad_exception();
			}
			for (auto& group : mProfileGroups)
			{
				group.SetPerformanceFrequency(mFrequency.QuadPart);
			}
		}
	}

	template<eOption tOption>
	constexpr void Profiler<tOption>::Begin(char const* const tag)
	{
		if constexpr (tOption == eOption::Enable)
		{
			if (gTLSThreadIndex == -1)
			{
				gTLSThreadIndex = InterlockedIncrement64(&mThreadIndexAcc);
				if (gTLSThreadIndex >= MAX_SUPPORT_THREAD)
				{
					throw std::logic_error("exceed support thread count");
				}
			}
			mProfileGroups[gTLSThreadIndex].StartRecord(tag);
		}
	}

	template<eOption tOption>
	constexpr void Profiler<tOption>::End(char const* const tag)
	{
		if constexpr (tOption == eOption::Enable)
		{
			if (gTLSThreadIndex == -1)
			{
				throw std::logic_error("invalid threa index");
			}
			mProfileGroups[gTLSThreadIndex].StopRecord(tag);
		}
	}

	template<eOption tOption>
	constexpr void Profiler<tOption>::Clear()
	{
		if constexpr (tOption == eOption::Enable)
		{
			for (auto& group : mProfileGroups)
			{
				group.Clear();
			}
		}
	}

	template<eOption tOption>
	constexpr void Profiler<tOption>::Dump()
	{
		if constexpr (tOption == eOption::Enable)
		{
			/*std::stringstream filenameStream;
			filenameStream << "profile" << InterlockedIncrement64(&mFileUIDAcc) << ".txt";
			std::ofstream file(filenameStream.str());
			Dump(file);*/
			Dump(std::cout);
		}
	}

	template<eOption tOption>
	void Profiler<tOption>::Dump(std::ostream& os)
	{
		os
			<< "ThreadID            |"
			<< "Name                |"
			<< "Elapsed (Avg)       |"
			<< "Elapsed (Min)       |"
			<< "Elapsed (Max)       |"
			<< "CallCount           |" << std::endl;
		for (int64_t i = 0; i <= mThreadIndexAcc; ++i)
		{
			os
				<< "--------------------+"
				<< "--------------------+"
				<< "--------------------+"
				<< "--------------------+"
				<< "--------------------+"
				<< "--------------------|" << std::endl
				<< mProfileGroups[i];
		}
	}
} // namespace nf::profile