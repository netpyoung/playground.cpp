#pragma once

#include <cstdint>
#include <thread>
#include <functional>

#include "../share/ErrorInfo.hpp"
#include "../share/ServerTimeInfo.hpp"

#include "../../common/lan/Packet.hpp"
#include "nf/network/common/Types.hpp"
#include "LanController.hpp"

using namespace nf::network::common::types;
using namespace nf::network::server::share;

namespace nf::network::server::lan
{
	class ILanCoreEvent
	{
	public:
		virtual ~ILanCoreEvent() = default;
	public:
		virtual void OnConnected(SessionInfo const& sessionInfo) = 0;	// OnClientJoin
		virtual void OnDisconnected(session_id const sessionID) = 0;	// OnClientLeave
		virtual void OnError(ErrorInfo const& errorInfo) = 0;
		virtual void OnReceived(session_id const sessionID, common::lan::Packet* const packet) = 0;
		virtual void OnWorkerThreadStarted(int32_t const networkIOThreadFuncID) = 0;
		virtual void OnWorkerThreadEnded(int32_t const networkIOThreadFuncID) = 0;
		//virtual void OnMonitorEvent()

		virtual void OnInitialized(NetController const& /*serverSender*/)
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
} // namespace nf::network::server::lan
