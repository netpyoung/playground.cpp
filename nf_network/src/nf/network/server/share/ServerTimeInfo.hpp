#pragma once

#include <cstdint>

namespace nf::network::server::share
{
	struct ServerTimeInfo
	{
		int32_t StartedMillisec;
		int32_t DeltaMillisec;
	};

} // namespace nf::network::server::share