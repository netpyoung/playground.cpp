#pragma once

#include <cstdint>

namespace chat
{
	enum class eJobType : uint32_t
	{
		UserLogin,
		UserSectorMove,
		UserMessage,
		UserHeartBeat,
		SystemDisconnectUser,
		SystemHeartBeat,
		INVALID,
	};
} // namespace chat