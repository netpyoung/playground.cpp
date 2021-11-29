#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <Windows.h>
#pragma warning(pop)

#include <cstdint>
#include <string>
#include <iostream>
#include <iomanip>
#include <array>
#include <thread>

namespace nf::profile
{
	constexpr size_t SKIP_MINMAX = 10;

	struct ProfileSample
	{
		ProfileSample();
		void Clear();
		friend std::ostream& operator<<(std::ostream& os, ProfileSample const& sample);

		bool IsActivated{ false };
		std::thread::id ThreadID;
		std::string Name = "";
		uint64_t CallCount{ 0 };
		::LARGE_INTEGER StartPerformanceCount{ 0, };
		int64_t SumOfElapsed{ 0 };
		std::array<int64_t, SKIP_MINMAX> MinElapseds;
		std::array<int64_t, SKIP_MINMAX> MaxElapseds;
	};
} // namespace nf::profile