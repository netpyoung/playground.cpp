#include "LanCore.hpp"

namespace nf::network::server::lan
{
	LanCore::LanCore(logging::Logger& logger) :
		LanCore(&logger)
	{
	}

	LanCore::LanCore(logging::Logger* const logger) :
		mLogger(logger)
	{
	}

	LanCore::~LanCore()
	{
		if (!mIsEnded)
		{
			Stop();
		}
	}

	// =======================================
	// API
	// =======================================

	bool LanCore::ValidateOption(ServerComponentOption const& option) const
	{
		if (option.NetworkIOThreadCount > 8)
		{
			return false;
		}
		return true;
	}

	std::optional<eServerComponentError> LanCore::Start(ServerComponentOption const& option)
	{
		// 옵션 검사하고,
		if (!ValidateOption(option))
		{
			return eServerComponentError::InvalidOption;
		}

		// 이벤트 핸들이 등록되었는지 검사해주자.
		if (!mEventHandler)
		{
			return eServerComponentError::NeedToRegisterEventHandler;
		}

		mServerComponentOption = option;

		{// 초기 기본 설정.
			// utf8 코드를 기본으로 간다.
			::SetConsoleOutputCP(define::UTF8_CODE_PAGE); // utf-8's CodePage.

			// 타이머를 정밀하게 해주자.
			::timeBeginPeriod(1);

			// TODO(pyoung): pool 크기 넘어가는경우 처리 안되어있음.
			network::common::lan::PacketPool::InitSendPool(define::DEFAULT_PACKET_MEMORY_POOL_SIZE);
			network::common::lan::PacketPool::InitRecvPool(define::DEFAULT_PACKET_MEMORY_POOL_SIZE);
			mSessionManager->Init(option.MaxSessionCount);
		}

		{// WSA 시작.
			::WSADATA wsaData;
			if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
			{
				mLogger->Error("ServerComponent", "WSAStartup");
				return eServerComponentError::Fail_WSAStartup;
			}
		}

		{// network IO용 iocp생성.
			::HANDLE const ioHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
			if (ioHandle == nullptr)
			{
				mLogger->Error("ServerComponent", "CreateIoCompletionPort : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_CreateIoCompletionPort_IOHandle;
			}
			mIOHandle = ioHandle;
		}

		{// 소켓 생성.
			// ref: https://www.slideshare.net/zone0000/ipv6-7542249
			::SOCKET serverSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (serverSock == INVALID_SOCKET)
			{
				mLogger->Error("ServerComponent", "socket(): %d", ::WSAGetLastError());
				return eServerComponentError::Fail_socket_serverSock;
			}
			mServerSock = serverSock;
		}

		{// 수신버퍼, 송신버퍼크기 0으로 설정.
			int32_t const optval = 0;
			//int32_t const resultRecv = ::setsockopt(mServerSock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const*>(&optval), sizeof(optval));
			//if (resultRecv == SOCKET_ERROR)
			//{
			//	mLogger->Error("ServerComponent", "setsockopt(SO_RCVBUF) : %d", ::WSAGetLastError());
			//	return eServerComponentError::Fail_setsockopt;
			//}
			int32_t const resultSend = ::setsockopt(mServerSock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char const*>(&optval), sizeof(optval));
			if (resultSend == SOCKET_ERROR)
			{
				mLogger->Error("ServerComponent", "setsockopt(SO_SNDBUF) : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_setsockopt;
			}
		}

		{// 소켓 바인딩.
			::SOCKADDR_IN sockAddr;
			::memset(&sockAddr, 0, sizeof(SOCKADDR_IN));
			sockAddr.sin_family = AF_INET;
			::InetPton(AF_INET, option.Address.c_str(), &sockAddr.sin_addr);
			sockAddr.sin_port = ::htons(option.Port);

			int32_t bindResult = ::bind(mServerSock, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr));
			if (bindResult == SOCKET_ERROR)
			{
				mLogger->Error("ServerComponent", "bind() : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_bind_serverSock;
			}
		}

		{// 서버 포트 오픈.
			int32_t listenResult = listen(mServerSock, SOMAXCONN);
			if (listenResult == SOCKET_ERROR)
			{
				mLogger->Error("ServerComponent", "listen() : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_listen_serverSock;
			}
		}

		{// 쓰레드 생성.
			// 네트워크 어셉트 쓰레드.
			mNetworkAcceptThread = std::thread(&LanCore::NetworkAcceptThread, this);

			// Send & Recv를 담당하는 I/O 쓰레드
			for (uint32_t i = 0; i < option.NetworkIOThreadCount; ++i)
			{
				mNetworkIOThreads.push_back(std::thread(&LanCore::NetworkIOThread, this, i, mIOHandle));
			}

			// 모니터링 쓰레드
			mMonitorRefreshThread = std::thread(&LanCore::MonitorRefreshThreadThread, this);
		}
		
		mLogger->Info("ServerComponent", "Server On : %s:%u", option.Address.c_str(), option.Port);
		return {};
	}

	bool LanCore::Stop()
	{
		mIsEnded = true;

		quitMre->Set();

		mLogger->Info("ServerComponent", "Stop() : starting");
		{//
			if (mMonitorRefreshThread.joinable())
			{
				mMonitorRefreshThread.join();
			}
		}
		{// Cleanup: NetworkAcceptThread.
			::closesocket(mServerSock);
			if (mNetworkAcceptThread.joinable())
			{
				mNetworkAcceptThread.join();
			}
		}

		{
			// TODO(pyoung): 셧다운 시도, 보낼게 있으면 보내기
			// TODO(pyoung): 셧다운 기다리기.
			// TODO(pyoung): PostQueuedCompletionStatus(handle, 0, 0, nullptr);로 처리하기.
			// TODO WokerThread종료기다리기
			if (mIOHandle != nullptr)
			{
				::CloseHandle(mIOHandle);
				mIOHandle = nullptr;
			}

			for (std::thread& thread : mNetworkIOThreads)
			{
				assert(thread.joinable());
				thread.join();
			}
		}

		common::lan::PacketPool::Clear();
		::WSACleanup();

		mLogger->Info("ServerComponent", "Stop() : stopped");

		mEventHandler->OnQuit();
		return true;
	}

	bool LanCore::Disconnect(session_id const sessionID)
	{
		
		manager::session::LanSession* const session = mSessionManager->FindSessionOrNull(sessionID);
		if (session == nullptr)
		{
			return false;
		}

		if (session->GetSessionID() != sessionID)
		{
			return false;
		}

		return TryRemoveSession(session);
	}

	// WIP
	void LanCore::Profile()
	{
		mProfiler.Dump();
	}

	void LanCore::Tick(int32_t const deltaMillisec)
	{
		mEventHandler->OnUpdate(ServerTimeInfo{ 0, deltaMillisec });
	}
} // namespace nf::network::server::lan