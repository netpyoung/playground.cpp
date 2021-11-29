#include "LanCore.hpp"

namespace nf::network::server::lan
{
	std::ostream& operator<<(std::ostream& os, monitor::MonitorInfo const& info)
	{
		return os
			<< "ConnectedSessionCount : " << info.ConnectedSessionCount << std::endl
			<< "AcceptTPS             : " << info.AcceptTPS << std::endl
			<< "AcceptTotal           : " << info.AcceptTotal << std::endl
			<< "RecvPacketTPS         : " << info.RecvPacketTPS << std::endl
			<< "SendPacketTPS         : " << info.SendPacketTPS << std::endl
			<< "PacketPoolSend        : " << info.PacketPoolSendUsedSize << "/" << info.PacketPoolSendMaxCapacity << std::endl
			<< "PacketPoolRecv        : " << info.PacketPoolRecvUsedSize << "/" << info.PacketPoolRecvMaxCapacity << std::endl;
	}

	void LanCore::MonitorRefreshThreadThread()
	{
		while (true)
		{
			if (quitMre->WaitOne(1000))
			{
				break;
			}
			monitor::MonitorInfo info;
			info.ConnectedSessionCount = mSessionManager->ConnectedSessionCount();
			info.AcceptTPS = InterlockedExchange(&mAcceptCountPerSec, 0);
			info.SendPacketTPS = InterlockedExchange(&mSendCountPerSec, 0);
			info.RecvPacketTPS = InterlockedExchange(&mRecvCountPerSec, 0);
			info.AcceptTotal = mAcceptTotal;
			info.PacketPoolSendMaxCapacity = common::lan::PacketPool::GetSendMaxCapacity();
			info.PacketPoolRecvMaxCapacity = common::lan::PacketPool::GetRecvMaxCapacity();
			info.PacketPoolSendUsedSize = common::lan::PacketPool::GetSendUsedSize();
			info.PacketPoolRecvUsedSize = common::lan::PacketPool::GetRecvUsedSize();
			std::cout << info << std::endl;
		}
	}
} // namespace nf::network::server::lan