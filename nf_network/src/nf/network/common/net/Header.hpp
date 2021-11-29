#pragma once

#include <cstdint>

namespace nf::network::common::net
{

#pragma pack(push, 1)
	struct NetHeader
	{
		uint8_t Code{ 0 };
		uint16_t Length{ 0 };
		uint8_t RandKey{ 0 };
		uint8_t CheckSum{ 0 };
	};
#pragma pack(pop)

	using Header = NetHeader;
} // namespace nf::network::common::net