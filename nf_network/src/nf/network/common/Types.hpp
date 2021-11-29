#pragma once

#include <cstdint>
#include <string>

namespace nf::network::common::types
{
	// TODO(pyoung): https://stackoverflow.com/questions/21295935/can-a-c-enum-class-have-methods

	enum class session_id : uint64_t
	{
		INVALID = 0
	};

	struct SessionInfo
	{
		session_id SessionID{ session_id::INVALID };
		std::string Address;
		uint16_t Port;
	};
} // namespace nf::network::common::types