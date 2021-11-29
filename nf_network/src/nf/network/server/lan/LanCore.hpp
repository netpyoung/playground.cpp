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

#include "nf/dump/CrashDump.hpp"
#include "../share/ErrorInfo.hpp"
#include "../share/eServerComponentError.hpp"
#include "ILanCoreEvent.hpp"
#include "../share/ServerTimeInfo.hpp"
#include "../share/ServerComponentOption.hpp"
#include <nf/network/define/Define.hpp>
#include "./manager/session/LanSessionManager.hpp"
#include "../monitor/MonitorInfo.hpp"
#include "nf/profile/Profiler.hpp"
#include "nf/network/common/Types.hpp"

using namespace nf::system::threading;
using namespace nf::network::common::types;

namespace nf::network::server::lan
{
	class LanCore
	{
	public:
		LanCore(logging::Logger* const logger);
		LanCore(logging::Logger& logger);
		virtual ~LanCore();
	public:
		LanCore(LanCore const& serverComponent) = delete;
		LanCore& operator=(LanCore const& serverComponent) = delete;
	public: // API

		template<typename T, typename... Args>
		void RegisterEventHandler(Args&&... args)
		{
			static_assert(std::is_base_of<ILanCoreEvent, T>::value, "T must be a descendant of IServerEvent");
			mEventHandler.reset(reinterpret_cast<ILanCoreEvent*>(new T(std::forward<Args>(args)...)));
			assert(mEventHandler != nullptr && "Fail to register");

			NetController controller;
			controller.SendPacket = [=](session_id const sessionID, common::lan::Packet* const packet) {
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

		bool SendPacket(session_id const sessionID, common::lan::Packet* const sendPacket);

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

		bool SendPost(manager::session::LanSession* const session);

		void RecvPost(manager::session::LanSession* const session);

		void NetworkRecvCompleted(manager::session::LanSession* const session, size_t const transferred);

		void NetworkSendCompleted(manager::session::LanSession* const session, size_t const transferred);

	private: // Etc
		manager::session::LanSession* CreateSessionOrNull(::SOCKET const sessionSock, ::SOCKADDR_IN const& sessionAddr);

		bool TryRemoveSession(manager::session::LanSession* const session);

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
		std::unique_ptr<manager::session::LanSessionManager> mSessionManager{ std::make_unique< manager::session::LanSessionManager>() };
		std::unique_ptr<ILanCoreEvent> mEventHandler;
		ServerComponentOption mServerComponentOption;
		uint64_t mRecvCountPerSec{ 0 };
		uint64_t mSendCountPerSec{ 0 };
		uint64_t mAcceptTotal{ 0 };
		uint64_t mAcceptCountPerSec{ 0 };

		std::unique_ptr<ManualResetEvent> const quitMre = std::make_unique<ManualResetEvent>(false);
		//std::unique_ptr<os::windows::thread::ManualResetEvent> mQuitMre{ std::make_unique<os::windows::thread::ManualResetEvent>(false) };

		profile::Profiler<profile::eOption::Enable> mProfiler;
		nf::dump::CrashDump mCrashDump;
	};
} // namespace nf::network::server::lan