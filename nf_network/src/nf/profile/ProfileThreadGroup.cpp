#include "ProfileThreadGroup.hpp"

namespace nf::profile
{
	void ProfileThreadGroup::StartRecord(char const* const tag)
	{
		for (auto& sample : mProfileSamples)
		{
			if (sample.Name.empty())
			{
				if (sample.IsActivated)
				{
					throw std::invalid_argument("Already activated tag");
				}

				sample.Name = tag;
				sample.ThreadID = std::this_thread::get_id();
				sample.IsActivated = true;
				::QueryPerformanceCounter(&sample.StartPerformanceCount);
				return;
			}

			if (sample.Name == tag)
			{
				if (sample.IsActivated)
				{
					throw std::invalid_argument("Already activated tag");
				}
				sample.IsActivated = true;
				::QueryPerformanceCounter(&sample.StartPerformanceCount);
				return;
			}
		}

		throw std::logic_error("Lack of ProfileSample space");
	}

	void ProfileThreadGroup::Clear()
	{
		for (auto& sample : mProfileSamples)
		{
			sample.Clear();
		}
	}

	void ProfileThreadGroup::StopRecord(char const* const tag)
	{
		::LARGE_INTEGER endStartPerformanceCount;
		::QueryPerformanceCounter(&endStartPerformanceCount);
		std::thread::id const currentThreadID = std::this_thread::get_id();
		for (auto& sample : mProfileSamples)
		{
			if (sample.ThreadID == currentThreadID && sample.Name == tag)
			{
				int64_t const diff = endStartPerformanceCount.QuadPart - sample.StartPerformanceCount.QuadPart;
				int64_t const elapsed = (diff * 10) / mFrequency;

				bool isNeedToUpdateSumOfElapsed = true;
				for (auto& min : sample.MinElapseds)
				{
					int64_t const curMin = std::min(min, elapsed);
					if (curMin != min)
					{
						min = elapsed;
						isNeedToUpdateSumOfElapsed = false;
						break;
					}
				}

				if (isNeedToUpdateSumOfElapsed)
				{
					for (auto& max : sample.MaxElapseds)
					{
						int64_t const curMax = std::max(max, elapsed);
						if (curMax != max)
						{
							max = elapsed;
							isNeedToUpdateSumOfElapsed = false;
							break;
						}
					}
				}

				if (isNeedToUpdateSumOfElapsed)
				{
					sample.SumOfElapsed = elapsed;
				}
				sample.CallCount++;
				sample.IsActivated = false;
				return;
			}
		}

		throw std::logic_error("Could not found tag name");
	}

	std::ostream& operator<<(std::ostream& os, ProfileThreadGroup const& grp)
	{
		for (auto& sample : grp.mProfileSamples)
		{
			if (sample.Name == "")
			{
				continue;
			}
			os << sample;
		}
		return os;
	}
} // namespace nf::profile