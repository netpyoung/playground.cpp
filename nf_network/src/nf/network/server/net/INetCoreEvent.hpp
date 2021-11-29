#pragma once

#include <cstdint>
#include <thread>
#include <functional>

#include "../share/ErrorInfo.hpp"
#include "../share/ServerTimeInfo.hpp"

#include "../../common/net/Packet.hpp"
#include "nf/network/common/Types.hpp"
#include "NetController.hpp"

using namespace nf::network::common::types;
using namespace nf::network::server::share;

namespace nf::network::server::net
{
	class INetCoreEvent
	{
	public:
		virtual ~INetCoreEvent() = default;
	public:
		virtual void OnConnected(SessionInfo const& sessionInfo) = 0;	// OnClientJoin
		virtual void OnDisconnected(session_id const sessionID) = 0;	// OnClientLeave
		virtual void OnError(ErrorInfo const& errorInfo) = 0;
		virtual void OnReceived(session_id const sessionID, common::net::Packet* const packet) = 0;
		virtual void OnWorkerThreadStarted(int32_t const networkIOThreadFuncID) = 0;
		virtual void OnWorkerThreadEnded(int32_t const networkIOThreadFuncID) = 0;
		//virtual void OnMonitorEvent()

		virtual void OnInitialized(NetController const& /*controller*/)
		{
		}
		virtual void OnUpdate(ServerTimeInfo const& /*serverTimeInfo*/)
		{
		}
		virtual void OnSent(session_id const /*sessionID*/, size_t const /*sendSize*/)
		{
		}
		virtual void OnQuit()
		{
		}
		virtual bool OnConnectionRequested(SessionInfo const& /*sessionInfo*/)
		{
			return true;
		}
	};
} // namespace nf::network::server::net
