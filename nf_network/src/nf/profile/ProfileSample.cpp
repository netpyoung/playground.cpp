#include "ProfileSample.hpp"

namespace nf::profile
{
	ProfileSample::ProfileSample()
	{
		Clear();
	}

	void ProfileSample::Clear()
	{
		IsActivated = false;
		Name = "";
		CallCount = 0;
		StartPerformanceCount.QuadPart = 0;
		SumOfElapsed = 0;
		for (auto& min : MinElapseds)
		{
			min = INT64_MAX;
		}
		for (auto& max : MaxElapseds)
		{
			max = 0;
		}
	}

	std::ostream& operator<<(std::ostream& os, ProfileSample const& sample)
	{
		os
			<< std::left
			<< std::setw(20) << sample.ThreadID << "|"
			<< std::setw(20) << sample.Name << "|"
			<< std::setw(20) << sample.SumOfElapsed / (long double)sample.CallCount * 1000000 << "|"
			<< std::setw(20) << sample.MinElapseds[0] / (long double)SKIP_MINMAX * 1000000 << "|"
			<< std::setw(20) << sample.MaxElapseds[0] / (long double)SKIP_MINMAX * 1000000 << "|"
			<< std::setw(20) << sample.CallCount << "|"
			<< std::endl;
		return os;
	}
} // namespace nf::profile