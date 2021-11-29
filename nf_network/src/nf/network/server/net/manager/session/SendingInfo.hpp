#pragma once

#include <nf/network/define/Define.hpp>
#include "nf/network/common/net/Packet.hpp"

namespace nf::network::server::net::manager::session
{
	struct SendingInfo
	{
		common::net::Packet* PacketBuffer[define::MAX_PACKET_COUNT_PER_SEND];
		int32_t PacketCount{ 0 };
		size_t TotalByteSize{ 0 };
		volatile int64_t IsSendingAtomic{ 0 };

		void Init()
		{
			PacketCount = 0;
			TotalByteSize = 0;
			InterlockedExchange64(&IsSendingAtomic, 0);
		}
	};
} // nf::network::server::net::manager::session
