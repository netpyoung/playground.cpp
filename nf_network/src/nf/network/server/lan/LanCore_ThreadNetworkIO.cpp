#include "LanCore.hpp"

// NetworkIOThread for (Send, Recv) 
namespace nf::network::server::lan
{
	// Send & Recv 통지
	void LanCore::NetworkIOThread(int32_t const networkIOThreadFuncID, ::HANDLE const ioHandle)
	{
		assert(ioHandle != nullptr);

		while (true)
		{
			mEventHandler->OnWorkerThreadStarted(networkIOThreadFuncID);
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
				{
					if (lastError == ERROR_ABANDONED_WAIT_0)
					{
						// NOTE(pyoung): ERROR_ABANDONED_WAIT_0인 경우는 `::CloseHandle(mIOHandle)`가 호출될때 발생.
						mLogger->Info("ServerComponent", "Shutdown NetoworkIOThread");
						mEventHandler->OnWorkerThreadEnded(networkIOThreadFuncID);
						return;
					}
					else
					{
						mLogger->Error("ServerComponent", "NetworkIOThread: lastError = %d", lastError);
						mEventHandler->OnWorkerThreadEnded(networkIOThreadFuncID);
						mCrashDump.Crash();
						return;
					}
				}
				else
				{
					// NOTE(pyoung): overlapped가 nullptr이 아닌경우, lpCompletionKey인 sessionAddr을 얻을 수 있다.
					manager::session::LanSession* const session = reinterpret_cast<manager::session::LanSession*>(sessionAddr);
					assert(session != nullptr);

					switch (lastError)
					{
					case ERROR_NETNAME_DELETED: // 64L
						// NOTE(pyoung): client가 이미 HardClose한 것이므로 로깅 안남김.
						break;
					default:
						// TODO(pyoung): session에 대한 정보도 로깅.
						mLogger->Error("ServerComponent", "NetworkIOThread: lastError = %d", lastError);
						break;
					}

					if (mSessionManager->DecrementIO(session) <= 0)
					{
						TryRemoveSession(session);
					}
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
			manager::session::LanSession* const session = reinterpret_cast<manager::session::LanSession*>(sessionAddr);
			assert(session != nullptr);

			// 정상종료인지 확인하고,
			if (transferred == 0)
			{
				// 정상종료 - 클라이언트 측이 closesocket() 혹은 shutdown().
				if (mSessionManager->DecrementIO(session) <= 0)
				{
					TryRemoveSession(session);
				}
				continue;
			}

			assert(transferred > 0);
			assert(overlapped != nullptr);
			assert(overlapped == &session->GetRecvOverlapped() || overlapped == &session->GetSendOverlapped());

			// GQCS의 통지가 Recv에서 온것인지, Send에서 온지 확인 후 처리.
			if (overlapped == &session->GetRecvOverlapped())
			{
				NetworkRecvCompleted(session, transferred);
			}
			else if (overlapped == &session->GetSendOverlapped())
			{
				NetworkSendCompleted(session, transferred);
			}

			if (mSessionManager->DecrementIO(session) <= 0)
			{
				TryRemoveSession(session);
			}

			mEventHandler->OnWorkerThreadEnded(networkIOThreadFuncID);
		}
	}

	bool LanCore::TryRemoveSession(manager::session::LanSession* const session)
	{
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
	bool LanCore::SendPacket(session_id const sessionID, common::lan::Packet* const sendPacket)
	{
		manager::session::LanSession* const session = mSessionManager->FindSessionOrNull(sessionID);
		if (session == nullptr)
		{
			mLogger->Debug("SendPacket", "where is session!");
			return false;
		}

		if (session->GetSessionID() != sessionID)
		{
			return false;
		}

		if (session->IsReleased())
		{
			//mLogger->Debug("SendPacket", "already released session: %llu", static_cast<uint64_t>(session->GetSessionID()));
			return false;
		}

		bool isSent = false;
		int64_t const increment = mSessionManager->IncrementIO(session);
		if (increment == 1)
		{
			// Pass
		}
		else if (session->GetSessionID() == sessionID)
		{
			// lan Packet 준비.
			network::common::lan::Header header;
			header.Length = static_cast<uint16_t>(sendPacket->GetBodySize());
			sendPacket->SetHeader(header);

			//
			bool const isEnqueue = session->Enqueue(sendPacket);
			if (isEnqueue)
			{
				mProfiler.Begin("SendPacket-SendPost");
				isSent = SendPost(session);
				mProfiler.End("SendPacket-SendPost");
			}
		}

		if (mSessionManager->DecrementIO(session) <= 0)
		{
			TryRemoveSession(session);
		}
		return isSent;
	}

	bool LanCore::SendPost(manager::session::LanSession* const session)
	{
		SendingBuffer sendingBuffer;
		if (!session->ReadySending(&sendingBuffer))
		{
			return false;
		}

		assert(sendingBuffer.BufferSize != 0);

		mSessionManager->IncrementIO(session);

		::DWORD dummyTransferred = 0;
		::DWORD dummyFlags = 0;
		int32_t const sendResult = ::WSASend(
			session->GetSocket(),
			sendingBuffer.Buffers.data(),
			sendingBuffer.BufferSize,
			&dummyTransferred,
			dummyFlags,
			sendingBuffer.Overlapped,
			nullptr);

		if (sendResult == SOCKET_ERROR)
		{
			int32_t const lastError = ::WSAGetLastError();
			if (lastError != WSA_IO_PENDING)
			{
				if (lastError != WSAECONNRESET)
				{
					mLogger->Error("SendPost", "WSASend : %d", lastError);
				}
				
				if (mSessionManager->DecrementIO(session) <= 0)
				{
					TryRemoveSession(session);
				}
				return false;
			}
		}

		InterlockedIncrement(&mSendCountPerSec);

		return true;
	}

	void LanCore::NetworkSendCompleted(manager::session::LanSession* const session, size_t const transferred)
	{
		session->FinishSending();
		mEventHandler->OnSent(session->GetSessionID(), transferred);
		SendPost(session);
	}

	// ==============================================================
	// Recv
	// ==============================================================
	void LanCore::RecvPost(manager::session::LanSession* const session)
	{
		size_t const directSize = session->GetRecvQueue().GetDirectEnqueueSize();
		size_t const enqueableSize = session->GetRecvQueue().GetEnquableSize();
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
		::DWORD dummyTransferred = 999;
		::DWORD dummyFlags = 0;
		int32_t const recvResult = ::WSARecv(
			session->GetSocket(),
			reinterpret_cast<WSABUF*>(wsaBuf),
			wsaBufSize,
			&dummyTransferred,
			&dummyFlags,
			&session->GetRecvOverlapped(),
			nullptr
			);

		if (recvResult == SOCKET_ERROR)
		{
			int32_t const lastError = ::WSAGetLastError();
			if (lastError == WSA_IO_PENDING)
			{
				return;
			}

			// WSAECONNABORTED			10053
			// WSAESHUTDOWN				10058
			// WSAENOTSOCK				10038
			// WSAECONNRESET			10054
			//if (lastError != WSAECONNRESET && lastError != WSAECONNABORTED)
			//{
			//	mLogger->Error("RecvPost", "WSARecv : %d", lastError);
			//}
			mLogger->Error("RecvPost", "WSARecv : %d", lastError);

			if (mSessionManager->DecrementIO(session) <= 0)
			{
				TryRemoveSession(session);
			}
		}
	}

	void LanCore::NetworkRecvCompleted(manager::session::LanSession* const session, size_t const transferred)
	{
		// IOCP로, RecvQueue는 이미 차 있으므로 WriteIndex를 옮겨주자.
		size_t const movedSize = session->GetRecvQueue().MoveWrite(transferred);
		assert(movedSize == transferred);
		if (movedSize < transferred)
		{
			mLogger->Warn("NetworkRecvCompleted", "[CloseSession] [%llu] Lack of space on RecvQueue : %llu / %llu", session->GetSessionID(), movedSize, transferred);
			session->CancelationIO();
			return;
		}

		while (true)
		{
			size_t const usedSize = session->GetRecvQueue().GetUsedSize();
			if (usedSize < common::lan::Packet::GetHeaderSize())
			{
				break;
			}

			common::lan::Header header;
			size_t const peekSize = session->GetRecvQueue().Peek(reinterpret_cast<std::byte*>(&header), common::lan::Packet::GetHeaderSize());
			assert(peekSize == common::lan::Packet::GetHeaderSize());
			assert(header.Length <= mServerComponentOption.MaxPacketBodyLength);

			if (header.Length > mServerComponentOption.MaxPacketBodyLength)
			{
				mLogger->Warn("NetworkRecvCompleted", "[CloseSession] [%llu] Invalid Header Length : %d", session->GetSessionID(), header.Length);
				session->CancelationIO();
				return;
			}

			if (usedSize < sizeof(header) + header.Length)
			{
				break;
			}

			session->GetRecvQueue().MoveRead(common::lan::Packet::GetHeaderSize());
			assert(session->GetRecvQueue().GetUsedSize() >= header.Length);

			{ // 패킷 완성 & 처리
				
				// 메모리풀에서 Packet할당(refCount = 1)
				common::lan::Packet* const recvPacket = common::lan::PacketPool::AllocRecv();
				
				// RecvQueue에서 Packet채우기.(Body부분만)
				size_t const dequeuedSize = session->GetRecvQueue().Dequeue(recvPacket->GetBodyPtr(), header.Length);
				assert(dequeuedSize == header.Length);
				recvPacket->MoveBodyWritePos(dequeuedSize);

				{ // 패킷 처리
					mProfiler.Begin("OnReceived");
					mEventHandler->OnReceived(session->GetSessionID(), recvPacket);
					mProfiler.End("OnReceived");
				}

				common::lan::PacketPool::TryFreeRecv(recvPacket);
			}

			InterlockedIncrement(&mRecvCountPerSec);
		}
		RecvPost(session);
	}

} // namespace nf::network::server::lan