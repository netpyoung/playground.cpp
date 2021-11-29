#include "LanCore.hpp"

namespace nf::network::server::lan
{
	void LanCore::NetworkAcceptThread()
	{
		while (true)
		{
			::SOCKADDR_IN sessionAddr{ 0, };
			int32_t addrSize = sizeof(::SOCKADDR_IN);

			// 블로킹 Accept.
			// NOTE(pyoung): WSAAcceptEX 도 고려해볼것.
			// ref: https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsaaccept
			::SOCKET const sessionSock = ::accept(mServerSock, reinterpret_cast<::sockaddr*>(&sessionAddr), &addrSize);
			if (sessionSock == INVALID_SOCKET)
			{
				// TODO(pyoung): Stop()에서 종료신호를 주고 기다렸다 종료시키면 이 에러는 안날꺼같은데...
				int32_t const lastError = ::WSAGetLastError();

				assert(lastError == WSAEINTR);

				mLogger->Error("ServerComponent", "accept() : %d", lastError);

				// WSAEINTR 10004
				// ref: https://docs.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
				break;
			}

			InterlockedIncrement(&mAcceptTotal);
			InterlockedIncrement(&mAcceptCountPerSec);
			
			// session용 소켓에 옵션 적용.
			std::optional<eServerComponentError> const err = ApplySocketOption(mLogger, sessionSock, mServerComponentOption);
			if (err)
			{
				::closesocket(sessionSock);
				continue;
			}

			// 세션 생성.
			manager::session::LanSession* const session = CreateSessionOrNull(sessionSock, sessionAddr);
			if (session == nullptr)
			{
				continue;
			}

			// Recv상태로 전환.
			RecvPost(session);
		}
	}

	std::optional<eServerComponentError> LanCore::ApplySocketOption(logging::Logger* const logger, ::SOCKET const sessionSock, ServerComponentOption const& option) const
	{
		{// 수신버퍼, 송신버퍼크기 0으로 설정.
			int32_t const optval = 0;
			//int32_t const resultRecv = ::setsockopt(sessionSock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<char const*>(&optval), sizeof(optval));
			//if (resultRecv == SOCKET_ERROR)
			//{
			//	logger->Error("ServerComponent", "setsockopt(SO_RCVBUF) : %d", ::WSAGetLastError());
			//	return eServerComponentError::Fail_setsockopt;
			//}
			int32_t const resultSend = ::setsockopt(sessionSock, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<char const*>(&optval), sizeof(optval));
			if (resultSend == SOCKET_ERROR)
			{
				logger->Error("ServerComponent", "setsockopt(SO_SNDBUF) : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_setsockopt;
			}
		}

		{// LINGER 설정.
			::linger optval;
			optval.l_onoff = 1;
			optval.l_linger = 0;
			int32_t const resultLinger = ::setsockopt(sessionSock, SOL_SOCKET, SO_LINGER, reinterpret_cast<char const*>(&optval), sizeof(optval));
			if (resultLinger == SOCKET_ERROR)
			{
				logger->Error("ServerComponent", "setsockopt(SO_LINGER) : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_setsockopt;
			}
		}

		{// Nagle알고리즘.
			int32_t const optval = option.IsEnableNagle ? 0 : 1;
			int32_t const resultNagle = ::setsockopt(sessionSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char const*>(&optval), sizeof(optval));
			if (resultNagle == SOCKET_ERROR)
			{
				logger->Error("ServerComponent", "setsockopt(TCP_NODELAY) : %d", ::WSAGetLastError());
				return eServerComponentError::Fail_setsockopt;
			}
		}

		//{// KEEP_ALIVE 옵션 설정.
		//	::tcp_keepalive tcpKeepAlive;
		//	tcpKeepAlive.onoff = 1;
		//	tcpKeepAlive.keepalivetime = 10 * 1000;
		//	tcpKeepAlive.keepaliveinterval = 1 * 1000;
		//	::DWORD dwBytesReturned;
		//	int32_t const resultTcpKeepalive = ::WSAIoctl(sessionSock,
		//		SIO_KEEPALIVE_VALS,
		//		&tcpKeepAlive,
		//		sizeof(tcpKeepAlive),
		//		0,
		//		0,
		//		&dwBytesReturned,
		//		nullptr,
		//		nullptr
		//	);
		//	if (resultTcpKeepalive == SOCKET_ERROR)
		//	{
		//		logger->Error("ServerComponent", "WSAIoctl(SIO_KEEPALIVE_VALS) : %d", ::WSAGetLastError());
		//		return eServerComponentError::Fail_WSAIoctl;
		//	}
		//}
		return {};
	}


	manager::session::LanSession* LanCore::CreateSessionOrNull(::SOCKET const sessionSock, ::SOCKADDR_IN const& sessionAddr)
	{
		constexpr size_t const IPADDR_SIZE = INET_ADDRSTRLEN; // INET6_ADDRSTRLEN

		char ipAddr[IPADDR_SIZE] = { 0, };
		::inet_ntop(AF_INET, reinterpret_cast<void const*>(&sessionAddr.sin_addr), ipAddr, IPADDR_SIZE);
		uint16_t const port = ::ntohs(sessionAddr.sin_port);

		SessionInfo sessionInfo
		{
			session_id::INVALID,
			ipAddr,
			port
		};

		if (!mEventHandler->OnConnectionRequested(sessionInfo))
		{
			return nullptr;
		}

		manager::session::LanSession* const session = mSessionManager->NewSessionOrNull(sessionSock, ipAddr, port);
		if (session == nullptr)
		{
			::closesocket(sessionSock);
			return nullptr;
		}

		::HANDLE const handle = ::CreateIoCompletionPort(reinterpret_cast<::HANDLE>(sessionSock), mIOHandle, reinterpret_cast<uint64_t>(session), 0);
		if (handle == nullptr)
		{
			mLogger->Error("ServerComponent", "CreateSessionOrNull");
			TryRemoveSession(session);
			return nullptr;
		}

		sessionInfo.SessionID = session->GetSessionID();

		mEventHandler->OnConnected(sessionInfo);

		return session;
	}
} // namespace nf::network::server::lan