#pragma once

#pragma warning(push, 0)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN // without MFC.
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib,"winmm.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Ws2def.h>
#include <Mstcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(pop)

#pragma warning(push, 0)
#include <vector>
#include <iostream>
#include <fstream>
#include <functional>
#include <thread>
#include <array>
#include <cassert>
#include <initializer_list>
#include <optional>
#pragma warning(pop)

#include <nf/logging/Logger.hpp>
#include <nf/system/threading/ManualResetEvent.hpp>
#include <nf/system/threading/AutoResetEvent.hpp>
#include <nf/buffer/ringbuffer/RingBuffer.hpp>
#include <nf/objectpool/ObjectPool.hpp>
#include <nf/assert/Assert.hpp>

#include "nf/dump/CrashDump.hpp"
#include "../share/ErrorInfo.hpp"
#include "../share/eServerComponentError.hpp"
#include "INetCoreEvent.hpp"

#include "../share/ServerTimeInfo.hpp"
#include "../share/ServerComponentOption.hpp"
#include <nf/network/define/Define.hpp>
#include "./manager/session/NetSessionManager.hpp"
#include "../monitor/MonitorInfo.hpp"
#include "../../common/net/Packet.hpp"
#include "nf/profile/Profiler.hpp"
#include "nf/network/common/Types.hpp"

using namespace nf::system::threading;
using namespace nf::network::server::share;
using namespace nf::network::common::types;

namespace nf::network::server::net
{
	class NetCore
	{
	public:
		NetCore(logging::Logger* const logger);
		NetCore(logging::Logger& logger);
		virtual ~NetCore();
	public:
		NetCore(NetCore const& serverComponent) = delete;
		NetCore& operator=(NetCore const& serverComponent) = delete;
	public: // API

		template<typename T, typename... Args>
		void RegisterEventHandler(Args&&... args)
		{
			static_assert(std::is_base_of<INetCoreEvent, T>::value, "T must be a descendant of IServerEvent");
			mEventHandler.reset(reinterpret_cast<INetCoreEvent*>(new T(std::forward<Args>(args)...)));
			assert(mEventHandler != nullptr && "Fail to register");

			NetController controller;
			controller.SendPacket = [=](session_id const sessionID, common::net::Packet* const packet) {
				return SendPacket(sessionID, packet);
			};
			controller.Disconnect = [=](session_id const sessionID) {
				return Disconnect(sessionID);
			};
			mEventHandler->OnInitialized(controller);
		}

		std::optional<eServerComponentError> Start(ServerComponentOption const& option);
		bool ValidateOption(ServerComponentOption const& option) const;

		bool Stop();

		bool Disconnect(session_id const sessionID);

		bool SendPacket(session_id const sessionID, common::net::Packet* const packet);

		void Profile();

	public: // inline API
		inline bool IsStopped() const
		{
			return mIsEnded;
		}
		inline bool IsRunning() const
		{
			return !mIsEnded;
		}
		inline size_t ConnectedSessionCount() const
		{
			return mSessionManager->ConnectedSessionCount();
		}
	public: // WIP
		void Tick(int32_t const deltaMillisec);

	private: // Threads
		void NetworkAcceptThread();

		std::optional<eServerComponentError> ApplySocketOption(logging::Logger* const logger, ::SOCKET const sessionSock, ServerComponentOption const& option) const;

		void NetworkIOThread(int32_t const networkIOThreadFuncID, ::HANDLE const ioHandle);

		bool SendPost(manager::session::NetSession* const session);

		void RecvPost(manager::session::NetSession* const session);

		bool NetworkRecvCompleted(manager::session::NetSession* const session, size_t const transferred);

	private: // Etc
		manager::session::NetSession* CreateSessionOrNull(::SOCKET const sessionSock, ::SOCKADDR_IN const& sessionAddr);

		manager::session::NetSession* AcquireSessionOrNull(session_id const sessionID);
		bool TryReleaseSession(manager::session::NetSession* const session);

		void MonitorRefreshThreadThread();

	private: // Member Variables.
		logging::Logger* const mLogger;
		std::thread mNetworkAcceptThread;
		std::vector<std::thread> mNetworkIOThreads; // TODO(pyoung): need to implment custom Array<T>
		std::thread mMonitorRefreshThread;
		::SOCKET mServerSock{ 0 };
		::HANDLE mIOHandle{ nullptr }; // TODO(pyoung): Apply SafeHandle.
		uint32_t mAccSessionID{ 0 };
		bool mIsEnded{ false };
		std::unique_ptr<manager::session::NetSessionManager> mSessionManager{ std::make_unique< manager::session::NetSessionManager>() };
		std::unique_ptr<INetCoreEvent> mEventHandler;
		ServerComponentOption mServerComponentOption;
		int64_t mRecvCountPerSec{ 0 }; // mRecvPacketCountPerSec
		int64_t mSendCountPerSec{ 0 }; // mSendCallCountPerSec
		uint64_t mAcceptTotal{ 0 };
		uint64_t mAcceptCountPerSec{ 0 };

		std::unique_ptr<ManualResetEvent> const quitMre = std::make_unique<ManualResetEvent>(false);
		profile::Profiler<profile::eOption::Enable> mProfiler;
		nf::dump::CrashDump mCrashDump;
	};
} // namespace nf::network::server::net