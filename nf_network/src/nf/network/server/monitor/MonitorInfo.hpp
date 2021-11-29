#pragma once

#include <iostream>
#include <cstdint>

namespace nf::network::server::monitor
{
	struct MonitorInfo
	{
	public:
		uint64_t ConnectedSessionCount{ 0 };
		uint64_t AcceptTPS{ 0 };
		uint64_t AcceptTotal{ 0 };
		uint64_t RecvPacketTPS{ 0 }; // 메시지당(모아서 보내니까)
		uint64_t SendPacketTPS{ 0 };
		uint64_t PacketPoolSendMaxCapacity{ 0 };
		uint64_t PacketPoolSendUsedSize{ 0 };
		uint64_t PacketPoolRecvMaxCapacity{ 0 };
		uint64_t PacketPoolRecvUsedSize{ 0 };
		uint64_t SessionCachedIndexes{ 0 };
		uint64_t SessionMaxIndexes{ 0 };
	};
} // namespace nf::network::server::monitor