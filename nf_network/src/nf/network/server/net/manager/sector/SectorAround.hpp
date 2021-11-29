#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
#pragma warning(pop)

#include "nf/mathematics/int2.hpp"

using namespace nf::mathematics;

namespace nf::network::server::net::manager::sector
{
#pragma warning(push)
#pragma warning(disable: 4820)
	struct SectorAround
	{	
		int32_t Count{ 0 };
		int2 Around[9];
	};
#pragma warning(pop)
} // namespace nf::network::server::net::manager::sector
