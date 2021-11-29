#pragma once

#include <cstdint>

namespace nf::network::server::share
{
	// NOTE(pyoung): interlocked64이용하니까 아래 구조 유지할것.
#pragma pack(push, 1)
	struct alignas(8) ReleaseInfo
	{
		volatile uint32_t IsReleasedAtomic{ 1 };
		volatile int32_t IOCountAtomic{ 0 };
	};
#pragma pack(pop)
} // namespace nf::network::server::share
