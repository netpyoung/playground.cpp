#pragma once

#include <cstdint>
#include <nf/network/common/net/Packet.hpp>
#include "eJobType.hpp"

using namespace nf;

namespace chat
{
	struct Job
	{
		Job(eJobType const jobType, session_id const sessionID, network::common::net::Packet* const packet) :
			JobType(jobType),
			SessionID(sessionID),
			RecvPacket(packet)
		{
		}
		~Job()
		{

		}

		eJobType JobType;
		session_id SessionID;
		network::common::net::Packet* RecvPacket;
	};
} // namespace chat