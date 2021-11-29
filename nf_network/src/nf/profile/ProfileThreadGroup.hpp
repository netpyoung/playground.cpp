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

#include "ProfileSample.hpp"

namespace nf::profile
{
	constexpr size_t MAX_SUPPORT_PROFILESAMPLE = 30;

	class ProfileThreadGroup final
	{
	public:
		ProfileThreadGroup() = default;
		~ProfileThreadGroup() = default;
		void StartRecord(char const* const tag);
		void StopRecord(char const* const tag);
		void Clear();
		friend std::ostream& operator<<(std::ostream& os, ProfileThreadGroup const& grp);
	public:
		void SetPerformanceFrequency(int64_t const frequency)
		{
			mFrequency = frequency;
		}
	private:
		int64_t mFrequency{ 1 };
		std::array<ProfileSample, MAX_SUPPORT_PROFILESAMPLE> mProfileSamples;
	};
} // namespace nf::profile