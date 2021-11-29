#pragma once

#include <cstdint>

namespace nf::network::common::lan
{
#pragma pack(push, 1)
	struct LanHeader
	{
		uint16_t Length{ 0 };
	};
#pragma pack(pop)

	using Header = LanHeader;
} // namespace nf::network::common::lan