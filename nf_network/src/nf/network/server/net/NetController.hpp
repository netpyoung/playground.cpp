#pragma once

#include <functional>

#include "../../common/net/Packet.hpp"
#include "nf/network/common/Types.hpp"

using namespace nf::network::common::types;

namespace nf::network::server::net
{
	struct NetController
	{
		std::function<bool(session_id const sessionID, common::net::Packet* const packet)> SendPacket;
		std::function<bool(session_id const sessionID)> Disconnect;
	};
} // namespace nf::network::server::net