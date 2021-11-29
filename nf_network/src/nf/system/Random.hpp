#pragma once

#include <cstdint>
#include <ctime>
#include <random>

namespace nf::system
{
	class Random final
	{
	public:
		Random(int32_t const seed)
		{
			mEngine.seed(seed);
		}
		~Random() = default;

	public:
		uint64_t rand(int32_t const max)
		{
			return mDistribution(mEngine) % max;
		}
	private:
		std::uniform_int_distribution<uint64_t> mDistribution{ 0, UINT64_MAX };
		std::mt19937_64 mEngine;
	};
} // namespace nf::system