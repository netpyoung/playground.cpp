#include "NetCore.hpp"

// NetworkIOThread for (Send, Recv) 
namespace nf::network::server::net
{
	// Send & Recv 통지
	void NetCore::NetworkIOThread(int32_t const /*networkIOThreadFuncID*/, ::HANDLE const ioHandle)
	{
		assert(ioHandle != nullptr);

		while (true)
		{
			assert(ioHandle != nullptr);

			// IOCP - GQCS확인
			// ref: https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getqueuedcompletionstatus
			uint64_t sessionAddr = 0;
			::DWORD transferred = 0; // NOTE(pyoung): 0 초기화 필수.
			::OVERLAPPED* overlapped = nullptr;
			bool const isSuccess = ::GetQueuedCompletionStatus(
				ioHandle,
				&transferred,
				&sessionAddr,
				&overlapped,
				INFINITE);

			//| return | lpNumberOfBytesTransferred | lpCompletionKey | lpOverlapped | GetLastError() |                  |
			//|--------|----------------------------|-----------------|--------------|----------------|------------------|
			//| FALSE  | x                          | x               | nullptr      |                |                  |
			//| FALSE  | set!                       | set!            | not nullptr  |                | failed operation |

			// 실패처리를 해주고,
			if (!isSuccess)
			{
				int32_t const lastError = ::WSAGetLastError();

				if (overlapped == nullptr)
				{ // 서버 종료 혹은 치명적 오류.
					if (lastError == ERROR_ABANDONED_WAIT_0)
					{
						// NOTE(pyoung): ERROR_ABANDONED_WAIT_0인 경우는 `::CloseHandle(mIOHandle)`가 호출될때 발생.
						mLogger->Info("NetworkIOThread", "lastError == ERROR_ABANDONED_WAIT_0");
						return;
					}
					else if (lastError == ERROR_INVALID_HANDLE)
					{
						// NOTE(pyoung): ERROR_INVALID_HANDLE인 경우는 `::CloseHandle(mIOHandle)`가 호출될때 발생.
						mLogger->Info("NetworkIOThread", "lastError == ERROR_INVALID_HANDLE");
						return;
					}

					mLogger->Error("NetworkIOThread", "[Crash] overlapped == nullptr : lastError = %d", lastError);
					mCrashDump.Crash();
					return;
				} // 서버 종료 혹은 치명적 오류.
				else
				{
					// NOTE(pyoung): overlapped가 nullptr이 아닌경우, lpCompletionKey인 sessionAddr을 얻을 수 있다.
					manager::session::NetSession* const session = reinterpret_cast<manager::session::NetSession*>(sessionAddr);
					if (session == nullptr)
					{
						mLogger->Error("NetworkIOThread", "session == nullptr (!isSuccess && overlapped != nullptr) lastError = %d", lastError);
						mCrashDump.Crash();
						continue;
					}

					if (session->IsReleased())
					{
						mLogger->Debug("NetworkIOThread", "Session Already Released(!isSuccess && overlapped != nullptr) lastError = %d", lastError);
						TryReleaseSession(session);
						continue;
					}

					switch (lastError)
					{
					case ERROR_NETNAME_DELETED: // 64L
						// NOTE(pyoung): client가 이미 HardClose한 것이므로 로깅 안남김.
						break;
					case ERROR_OPERATION_ABORTED: // 995L
						// The I/O operation has been aborted because of either a thread exit or an application request.
						// CancelIoEx를 사용함으로 그것에 대한 중단이 `ERROR_OPERATION_ABORTED`이다.
						break;
					case ERROR_CONNECTION_ABORTED: // 1236L
						// TODO(pyoung): ERROR_CONNECTION_ABORTED ??? - 로직에서 그냥 클로즈소켓을 때려버리는 경우 이리로 발생.... // keepalive?
						mLogger->Debug("NetworkIOThread", "ERROR_CONNECTION_ABORTED keepalive?");
						break;
					case WSAECONNABORTED: // 10053L
						// An established connection was aborted by the software in your host machine. // keepalive?
						mLogger->Debug("NetworkIOThread", "WSAECONNABORTED keepalive?");
						break;
					case ERROR_SEM_TIMEOUT: // 121L
						// release-0-0에서 발생
						// 세마포어에러. TCP스택에서 운영체제가 끝어버린거임 - 어쩔 수 없지만, 정상적인 종료랑 구분할수 있어야 함으로, 로깅을 해야함.
						// 코드최적화나 인텔랜카드(하드웨어 지원).
						// 클라가 종료시 RST만 보낼려고 했는데 전송되지 않아, 서버가 오류로 판단하는 경우도 있음.
						mLogger->Error("NetworkIOThread", "NetCore::NetworkIOThread - ERROR_SEM_TIMEOUT");
						session->CancelationIO(); // NetCore::NetworkIOThread - ERROR_SEM_TIMEOUT

						break;
					default:
						mLogger->Error("NetworkIOThread", "[Crash] NetworkIOThread: lastError = %d", lastError);
						Sleep(1000);
						mCrashDump.Crash();
						break;
					}

					TryReleaseSession(session);
					continue;
				}
			}

			// IOCP로부터 통지를 받아오는 것을 성공하였으면,
			//| return | lpNumberOfBytesTransferred | lpCompletionKey | lpOverlapped |                                                          |
			//|--------|----------------------------|-----------------|--------------|----------------------------------------------------------|
			//| TRUE   | set!                       | set!            | set!         |                                                          |
			//| TRUE   | 0                          | set!            | set!         | 정상종료 - 클라이언트 측이 closesocket() 혹은 shutdown() |

			assert(isSuccess);
			assert(sessionAddr != 0);

			// 세션을 얻어온다.
			manager::session::NetSession* const session = reinterpret_cast<manager::session::NetSession*>(sessionAddr);
			if (session == nullptr)
			{
				mLogger->Error("NetworkIOThread", "session == nullptr (isSuccess && overlapped != nullptr)");
				mCrashDump.Crash();
				continue;
			}

			if (session->IsReleased())
			{
				mLogger->Debug("NetworkIOThread", "Session Already Released(isSuccess && overlapped != nullptr)");
				continue;
			}

			// 정상종료인지 확인하고,
			if (transferred == 0)
			{
				TryReleaseSession(session);
				continue;
			}

			if (session->IsClosed())
			{
				// 누군가에 의해 CloseSocketOnce이 불려졌다(CancelIoEx).
				TryReleaseSession(session);
				continue;
			}

			if (overlapped == &session->GetSendOverlapped())
			{
				int64_t const sendCount = session->FinishSending();
				InterlockedAdd64(&mSendCountPerSec, sendCount);
				mEventHandler->OnSent(session->GetSessionID(), transferred);
				SendPost(session);
				TryReleaseSession(session);
				continue;
			}

			if (overlapped == &session->GetRecvOverlapped())
			{
				bool const isContinueRecv = NetworkRecvCompleted(session, transferred);
				if (isContinueRecv)
				{
					RecvPost(session);
				}
				TryReleaseSession(session);
				continue;
			}

			// NOTE(pyoung): GQCS에서 받은 overlapped가 세션의 overlapped가 아닌경우.
			// session 종료처리를 제대로 안하면, 소켓재활용하면서? 이리로 떨어질 가능성 이 있다.
			// logging후 크래쉬.
			mLogger->Error("NetworkIOThread", "WTF");
			mCrashDump.Crash();
		}
	}

	manager::session::NetSession* NetCore::AcquireSessionOrNull(session_id const sessionID)
	{
		manager::session::NetSession* const session = mSessionManager->FindSessionOrNull(sessionID);
		if (session == nullptr)
		{
			return nullptr;
		}

		if (mSessionManager->IncrementIO(session) == 1)
		{
			TryReleaseSession(session);
			return nullptr;
		}

		if (session->GetSessionID() != sessionID)
		{
			TryReleaseSession(session);
			return nullptr;
		}

		if (session->IsReleased())
		{
			TryReleaseSession(session);
			return nullptr;
		}
		return session;
	}

	bool NetCore::TryReleaseSession(manager::session::NetSession* const session)
	{
		int64_t const ioDecremented = mSessionManager->DecrementIO(session);
		if (ioDecremented < 0)
		{
			throw std::logic_error("ioCount under 0");
		}

		if (ioDecremented != 0)
		{
			return false;
		}

		session_id const sessionID = session->GetSessionID();
		if (!mSessionManager->TryReleaseSession(session))
		{
			return false;
		}

		mEventHandler->OnDisconnected(sessionID);
		return true;
	}

	// ==============================================================
	// Send
	// ==============================================================
	bool NetCore::SendPacket(session_id const sessionID, common::net::Packet* const sendPacket)
	{
		manager::session::NetSession* const session = AcquireSessionOrNull(sessionID);
		if (session == nullptr)
		{
			return false;
		}

		sendPacket->EncodeOnce(mServerComponentOption.PacketCode, mServerComponentOption.PacketEncodeKey);
		bool isSent = false;
		if (session->Enqueue(sendPacket))
		{
			isSent = SendPost(session);
		}
		TryReleaseSession(session);
		return isSent;
	}

	bool NetCore::SendPost(manager::session::NetSession* const session)
	{
		SendingBuffer sendingBuffer;
		if (!session->ReadySending(&sendingBuffer))
		{
			return false;
		}

		mSessionManager->IncrementIO(session);
		session->ResetSendOverlapped();

		::DWORD dummyFlags = 0;
		::SOCKET const sessionSocket = session->GetSocket();
		if (sessionSocket == INVALID_SOCKET)
		{
			TryReleaseSession(session);
			return false;
		}

		int32_t const sendResult = ::WSASend(
			sessionSocket,
			sendingBuffer.Buffers.data(),
			sendingBuffer.BufferSize,
			NULL,
			dummyFlags,
			sendingBuffer.Overlapped,
			nullptr);

		if (sendResult == 0)
		{
			return true;
		}

		int32_t const lastError = ::WSAGetLastError();
		if (lastError == WSA_IO_PENDING)
		{
			return true;
		}

		switch (lastError)
		{
		case WSAEINTR: // 10004L
			break;
		case WSAECONNABORTED: // 10053L
			[[fallthrough]];
		case WSAEINVAL: // 10022L
			[[fallthrough]];
		case WSAESHUTDOWN: // 10058L
			mLogger->Debug("SendPost", "lastError = %d | isInvalidSocket? = %d", lastError, sessionSocket == INVALID_SOCKET);
			break;
		case WSAECONNRESET: // 10054L
			// An existing connection was forcibly closed by the remote host.
			[[fallthrough]];
		case WSAENOTSOCK: // 10038L
			break;
		case WSANOTINITIALISED: // 10093L
			mLogger->Error("SendPost", "WSANOTINITIALISED - termination");
			break;
		default:
			auto& sendQueue = session->GetSendQueue();
			mLogger->Error("SendPost", "WSASend| lastError: %d | index: %u | uid: %llu | ioCount : %d | sendQueue : %llu / %llu",
				lastError,
				session->Index(), session->UID(), session->GetIOCount(),
				sendQueue.GetUsedSize(),
				sendQueue.GetMaxCapacity());
			mCrashDump.Crash();
			break;
		}

		TryReleaseSession(session);
		return false;
	}

	// ==============================================================
	// Recv
	// ==============================================================
	void NetCore::RecvPost(manager::session::NetSession* const session)
	{
		size_t const enqueableSize = session->GetRecvQueue().GetEnquableSize();
		if (enqueableSize == 0)
		{
			mLogger->Debug("RecvPost", "[CloseSession] [%llu] RecvQueue is fulled", session->GetSessionID());
			session->CancelationIO(); // NetCore::RecvPost: (enqueableSize == 0)
			return;
		}

		size_t const directSize = session->GetRecvQueue().GetDirectEnqueueSize();
		assert(directSize != 0);
		assert(enqueableSize != 0);

		::WSABUF wsaBuf[2];
		::DWORD wsaBufSize = 0;
		if (directSize == enqueableSize)
		{
			wsaBufSize = 1;
			wsaBuf[0].buf = reinterpret_cast<char*>(session->GetRecvQueue().GetWriteBufferPtr());
			wsaBuf[0].len = static_cast<uint32_t>(directSize);
		}
		else
		{
			wsaBufSize = 2;
			wsaBuf[0].buf = reinterpret_cast<char*>(session->GetRecvQueue().GetWriteBufferPtr());
			wsaBuf[1].buf = reinterpret_cast<char*>(session->GetRecvQueue().GetBeginBufferPtr());
			wsaBuf[0].len = static_cast<uint32_t>(directSize);
			wsaBuf[1].len = static_cast<uint32_t>(enqueableSize - directSize);
		}

		assert(wsaBufSize != 0);

		mSessionManager->IncrementIO(session);

		session->ResetRecvOverlapped();

		::DWORD dummyFlags = 0;
		::SOCKET const sessionSocket = session->GetSocket();
		if (sessionSocket == INVALID_SOCKET)
		{
			TryReleaseSession(session);
			return;
		}

		int32_t const recvResult = ::WSARecv(
			sessionSocket,
			reinterpret_cast<WSABUF*>(wsaBuf),
			wsaBufSize,
			NULL,
			&dummyFlags,
			&session->GetRecvOverlapped(),
			nullptr
		);

		if (recvResult != SOCKET_ERROR)
		{
			return;
		}

		int32_t const lastError = ::WSAGetLastError();
		if (lastError == WSA_IO_PENDING)
		{
			return;
		}

		switch (lastError)
		{
		case WSAECONNABORTED: // 10053L
			[[fallthrough]];
		case WSAESHUTDOWN: // 10058L
			[[fallthrough]];
		case WSAEINTR: // 10004L
			mLogger->Debug("RecvPost", "lastError = %d", lastError);
			break;
		case WSAECONNRESET: // 10054L
			[[fallthrough]];
		case WSAENOTSOCK: // 10038L
			break;
			// TODO(pyoung): WSAENOBUFS이거 나오면 소켓 끊어줘야함.
		default:
			mLogger->Debug("RecvPost", "lastError = %d", lastError);
			auto& recvQueue = session->GetRecvQueue();
			mLogger->Error("RecvPost", "WSARecv| lastError: %d | index: %u | uid: %llu | ioCount : %d | recvQueue : %llu / %llu",
				lastError,
				session->Index(), session->UID(), session->GetIOCount(),
				recvQueue.GetUsedSize(),
				recvQueue.GetMaxCapacity());
			mCrashDump.Crash();
			break;
		}

		TryReleaseSession(session);
	}

	bool NetCore::NetworkRecvCompleted(manager::session::NetSession* const session, size_t const transferred)
	{
		// IOCP로, RecvQueue는 이미 차 있으므로 WriteIndex를 옮겨주자.
		size_t const movedSize = session->GetRecvQueue().MoveWrite(transferred);
		if (movedSize != transferred)
		{
			mLogger->Debug("NetworkRecvCompleted", "[CloseSession] [%llu | index: %u | uid: %llu | ioCount : %d] Lack of space on RecvQueue : %llu / %llu",
				session->GetSessionID(),
				session->Index(), session->UID(), session->GetIOCount(),
				movedSize, transferred);
			session->CancelationIO(); // NetCore::NetworkRecvComplete: (movedSize != transferred)
			return false;
		}

		while (true)
		{
			size_t const usedSize = session->GetRecvQueue().GetUsedSize();
			if (usedSize < common::net::Packet::GetHeaderSize())
			{
				// 기본적인 헤더를 완성할 충분할 데이터를 받지 못했다. 한번더 recv걸도록 break;
				return true;
			}

			common::net::Header header;
			size_t const peekSize = session->GetRecvQueue().Peek(reinterpret_cast<std::byte*>(&header), common::net::Packet::GetHeaderSize());
			assert(peekSize == common::net::Packet::GetHeaderSize());
			assert(header.Length <= mServerComponentOption.MaxPacketBodyLength);

			if (header.Code != mServerComponentOption.PacketCode)
			{
				mLogger->Debug("NetworkRecvCompleted", "[CloseSession] [%llu | index: %u | uid: %llu | ioCount : %d] Invalid Header Code : %u",
					session->GetSessionID(),
					session->Index(), session->UID(), session->GetIOCount(),
					header.Code);
				session->CancelationIO(); // NetCore::NetworkRecvComplete: (header.Code != mServerComponentOption.PacketCode)
				return false;
			}

			if (header.Length > mServerComponentOption.MaxPacketBodyLength || header.Length >= common::net::Packet::GetBodyCapacity())
			{
				mLogger->Debug("NetworkRecvCompleted", "[CloseSession] [%llu | index: %u | uid: %llu | ioCount : %d] Invalid Header Length : %u",
					session->GetSessionID(),
					session->Index(), session->UID(), session->GetIOCount(),
					header.Length);
				session->CancelationIO(); // NetCore::NetworkRecvComplete: (header.Length > mServerComponentOption.MaxPacketBodyLength || header.Length >= common::net::Packet::GetBodyCapacity())
				return false;
			}

			size_t const desirePacketSize = sizeof(header) + header.Length;

			if (desirePacketSize > usedSize)
			{
				// 패킷을 완성할 충분할 데이터를 받지 못했다. 한번더 recv걸도록 break;
				return true;
			}

			{ // 패킷 완성 & 처리

				// 메모리풀에서 Packet할당(recvPacket's refCount +1)
				common::net::Packet* const recvPacket = common::net::PacketPool::AllocRecv();

				// RecvQueue에서 Packet채우기.(Body부분만)
				size_t const dequeuedSize = session->GetRecvQueue().Dequeue(recvPacket->GetHeaderPtr(), desirePacketSize);
				if (dequeuedSize != desirePacketSize)
				{
					mLogger->Error("NetworkRecvCompleted", "Failed Dequeue : %zu", dequeuedSize);
					mCrashDump.Crash();
					return false;
				}

				size_t const movedWritePos = recvPacket->MoveBodyWritePos(header.Length);
				if (movedWritePos != header.Length)
				{
					mLogger->Error("NetworkRecvCompleted", "Failed MoveBodyWritePos : %zu != %u", movedWritePos, header.Length);
					mCrashDump.Crash();
					return false;
				}

				bool const isDecoded = recvPacket->DoDecode(mServerComponentOption.PacketEncodeKey, header.RandKey);
				if (!isDecoded)
				{
					common::net::PacketPool::TryFreeRecv(recvPacket); // (recvPacket's refCount -1)
					// 위조된 패킷
					mLogger->Debug("NetworkRecvCompleted", "[CloseSession] [%llu | index: %u | uid: %llu | ioCount : %d] Failed To Decode",
						session->GetSessionID(),
						session->Index(), session->UID(), session->GetIOCount()
					);
					session->CancelationIO(); // NetCore::NetworkRecvComplete: (!isDecoded)
					return false;
				}

				{ // 패킷 처리
					//mProfiler.Begin("OnReceived");
					try
					{
						mEventHandler->OnReceived(session->GetSessionID(), recvPacket);
					}
					catch (nf::network::packet::PacketReaderException const& /*pre*/)
					{
						// FIXME(pyoung): recvPacket의 refCount를 강제로 0으로 만들어서 제대로 회수되도록 만들어 주어야 함.
						common::net::PacketPool::TryFreeRecv(recvPacket); // (recvPacket's refCount -1)

						//mProfiler.End("OnReceived");
						mLogger->Debug("NetworkRecvCompleted", "[CloseSession] [%llu | index: %u | uid: %llu | ioCount : %d] PacketReaderException",
							session->GetSessionID(),
							session->Index(), session->UID(), session->GetIOCount()
						);
						session->CancelationIO(); // NetCore::NetworkRecvComplete: (nf::network::packet::PacketReaderException const& /*pre*/)
						return false;
					}
					catch (std::runtime_error const& re)
					{
						mLogger->Error("NetworkRecvCompleted", "Runtime error: %s", re.what());
						mCrashDump.Crash();
						return false;
					}
					catch (std::exception const& ex)
					{
						mLogger->Error("NetworkRecvCompleted", "Error occurred: %s", ex.what());
						mCrashDump.Crash();
						return false;
					}
					catch (...)
					{
						mLogger->Error("NetworkRecvCompleted", "WTF catch");
						mCrashDump.Crash();
						return false;
					}
					//mProfiler.End("OnReceived");
				} // 패킷 처리

				common::net::PacketPool::TryFreeRecv(recvPacket); // (recvPacket's refCount -1)
			} // 패킷 완성 & 처리

			InterlockedIncrement64(&mRecvCountPerSec);
		} // while
	}

} // namespace nf::network::server::net