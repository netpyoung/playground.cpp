#pragma once

#include <nf/network/define/Define.hpp>
#include "nf/network/common/lan/Packet.hpp"

namespace nf::network::server::lan::manager::session
{
	struct SendingInfo
	{
		common::lan::Packet* PacketBuffer[define::MAX_PACKET_COUNT_PER_SEND];
		int32_t PacketCount{ 0 };
		size_t TotalByteSize{ 0 };
		int64_t IsSendingAtomic{ 0 };
	};
} // nf::network::server::lan::manager::session
