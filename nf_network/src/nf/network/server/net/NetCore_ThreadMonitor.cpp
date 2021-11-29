#include "NetCore.hpp"

namespace nf::network::server::net
{
	std::ostream& operator<<(std::ostream& os, monitor::MonitorInfo const& info)
	{
		return os
			<< "ConnectedSessionCount : " << info.ConnectedSessionCount << std::endl
			<< "SessionManager        : " << info.SessionCachedIndexes << "/" << info.SessionMaxIndexes << std::endl
			<< "AcceptTPS             : " << info.AcceptTPS << std::endl
			<< "AcceptTotal           : " << info.AcceptTotal << std::endl
			<< "SendPacketTPS         : " << info.SendPacketTPS << std::endl
			<< "RecvPacketTPS         : " << info.RecvPacketTPS << std::endl
			<< "PacketPoolSend        : " << info.PacketPoolSendUsedSize << "/" << info.PacketPoolSendMaxCapacity << std::endl
			<< "PacketPoolRecv        : " << info.PacketPoolRecvUsedSize << "/" << info.PacketPoolRecvMaxCapacity << std::endl;
	}

	void NetCore::MonitorRefreshThreadThread()
	{
		while (true)
		{
			if (quitMre->WaitOne(1000))
			{
				break;
			}
			monitor::MonitorInfo info;
			info.ConnectedSessionCount = mSessionManager->ConnectedSessionCount();
			info.SessionCachedIndexes = mSessionManager->GetCachedSessionCount();
			info.SessionMaxIndexes = mSessionManager->GetSessionMaxCapacity();
			info.AcceptTPS = InterlockedExchange(&mAcceptCountPerSec, 0);
			info.SendPacketTPS = InterlockedExchange64(&mSendCountPerSec, 0);
			info.RecvPacketTPS = InterlockedExchange64(&mRecvCountPerSec, 0);
			info.AcceptTotal = mAcceptTotal;
			info.PacketPoolSendMaxCapacity = common::net::PacketPool::GetSendMaxCapacity();
			info.PacketPoolRecvMaxCapacity = common::net::PacketPool::GetRecvMaxCapacity();
			info.PacketPoolSendUsedSize = common::net::PacketPool::GetSendUsedSize();
			info.PacketPoolRecvUsedSize = common::net::PacketPool::GetRecvUsedSize();
			std::cout << info << std::endl;
		}
	}
} // namespace nf::network::server::net