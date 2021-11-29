#include "Profiler.hpp"

namespace nf::profile
{
	extern __declspec(thread) int64_t gTLSThreadIndex = -1;
} // namespace nf::profile
