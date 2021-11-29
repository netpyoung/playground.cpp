#pragma once

#include <cstdint>
#include <functional>

#include "../../common/lan/Packet.hpp"
#include "nf/network/common/Types.hpp"

using namespace nf::network::common::types;

namespace nf::network::server::lan
{
	struct NetController
	{
		std::function<bool(session_id const sessionID, common::lan::Packet* const packet)> SendPacket;
		std::function<bool(session_id const sessionID)> Disconnect;
	};
} // namespace nf::network::server::lan
