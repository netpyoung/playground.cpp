#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
#include <WinSock2.h>
#include <timeapi.h>
#include <string>
#include <cassert>
#include <atomic>
#include <stdexcept>
#include <array>
#pragma warning(pop)

#include "nf/buffer/ringbuffer/RingBuffer.hpp"
#include "nf/network/define/Define.hpp"
#include "nf/network/server/share/SendingBuffer.hpp"
#include "nf/network/common/net/Packet.hpp"
#include "nf/network/common/Types.hpp"
#include "nf/system/collections/concurrent/LockFreeQueue.hpp"
#include "SendingInfo.hpp"
#include "../../../share/eSessionSocketState.hpp"
#include "../../../share/ReleaseInfo.hpp"
#include "nf/logging/Logger.hpp"

using namespace nf::network::server::share;
using namespace nf::network::common::types;

namespace nf::network::server::net::manager::session
{
	class NetSessionManager;

	class NetSession final
	{
	public:
		NetSession(uint32_t const sessionIndex, nf::logging::Logger* const logger);
		virtual ~NetSession();
	public:
		NetSession(NetSession const& session) = delete;
		NetSession& operator=(NetSession const& session) = delete;

	public:
		inline static session_id GenSessionID(uint32_t const index, uint64_t const uid)
		{
			uint64_t sessionID = index;
			sessionID <<= 48;
			sessionID |= (uid & 0x0000FFFFFFFFFFFF);
			return static_cast<session_id>(sessionID);
		}

		inline static uint32_t SessionIndex(session_id const sid)
		{
			return static_cast<uint32_t>(static_cast<uint64_t>(sid) >> 48);
		}

		inline static uint64_t SessionUID(session_id const sid)
		{
			return (static_cast<uint64_t>(sid) & 0x0000FFFFFFFFFFFF);
		}

	public:
		// Init -> Enqueue -> ReadySending -> FinishSending -> TryRelease
		void Init(SOCKET const sock, char const* const address, uint16_t const port);
		bool Enqueue(common::net::Packet* const sendPacket);
		bool ReadySending(SendingBuffer* const sendingBuffer);
		int32_t FinishSending();
		bool TryRelease();
	public:
		inline uint32_t Index() const
		{
			return mSessionIndex;
		}

		inline uint64_t UID() const
		{
			return NetSession::SessionUID(mNetSessionID);
		}

		inline uint64_t PrevUID() const
		{
			return mPrevUID;
		}

		inline SOCKET GetSocket() const
		{
			if (IsClosed())
			{
				return INVALID_SOCKET;
			}
			return mSocket;
		}

		inline session_id GetSessionID() const
		{
			return mNetSessionID;
		}

		inline uint32_t GetRecvTime() const
		{
			return mRecvTime;
		}

		inline buffer::ringbuffer::RingBuffer& GetRecvQueue()
		{
			return mRecvQueue;
		}

		inline system::collections::concurrent::LockFreeQueue<common::net::Packet*>& GetSendQueue()
		{
			return mSendQueue;
		}

		inline void SetRecvTime(uint32_t const recvTime)
		{
			mRecvTime = recvTime;
		}

		inline::OVERLAPPED& GetRecvOverlapped()
		{
			return mRecvOverlapped;
		}

		inline void ResetSendOverlapped()
		{
			::memset(&mSendOverlapped, 0, sizeof(::OVERLAPPED));
		}

		inline void ResetRecvOverlapped()
		{
			::memset(&mRecvOverlapped, 0, sizeof(::OVERLAPPED));
		}

		inline::OVERLAPPED& GetSendOverlapped()
		{
			return mSendOverlapped;
		}

		inline bool IsReleased() const
		{
			return mReleaseInfo.IsReleasedAtomic == 1;
		}

		inline std::pair<bool, ::DWORD> CancelationIO()
		{
			if (::InterlockedCompareExchange(&mIsCloseSocket, 1, 0) == 0)
			{
				// NOTE(pyoung): `closesocket`은 CancelIoEx후 ioCount가 0이될때 Release가 호출되면서 호출할 것이다.
				// NOTE(pyoung): `shutdown` 안쓰기.
				// - shutdown에서는 TIME_WAIT은 피할 수 없음.
				// - shutdown은 FIN을 보냄.
				// - FIN에 대한 ACK를 못받을 수 있음(크래쉬된 클라있으면 ACK못받는 것임.이것은 CancelIO로 회피가능)

				// If this parameter is NULL, all I/O requests for the hFile parameter are canceled.
				if (::CancelIoEx(reinterpret_cast<::HANDLE>(mSocket), NULL) == 0)
				{
					int32_t const lastError = ::GetLastError();
					return { false, lastError };
				}
				return { true, ERROR_SUCCESS };
			}

			return { false, ERROR_SUCCESS };
		}

		inline bool IsSendRecvable() const
		{
			return (!IsClosed() || !IsReleased());
		}

		inline bool IsClosed() const
		{
			return mIsCloseSocket == 1;
		}

		inline int32_t GetIOCount() const
		{
			return mReleaseInfo.IOCountAtomic;
		}
	protected:
		// NOTE(pyoung): ioCount는 friend인 SessionManager만 접근할 수 있도록 한다.
		inline int32_t IncrementIOCount()
		{
			return ::InterlockedIncrement(reinterpret_cast<volatile uint32_t*>(&mReleaseInfo.IOCountAtomic));
		}

		inline int32_t DecrementIOCount()
		{
			return ::InterlockedDecrement(reinterpret_cast<volatile uint32_t*>(&mReleaseInfo.IOCountAtomic));
		}

	private:
		friend NetSessionManager;
	private:
		void Release();
	private:
		alignas(16) ReleaseInfo mReleaseInfo;
		uint32_t mSessionIndex;
		::SOCKET mSocket{ INVALID_SOCKET };
		session_id mNetSessionID{ session_id::INVALID };
		std::string mAddress{ "" };
		uint16_t mPort{ 0 };

		::OVERLAPPED mRecvOverlapped{ 0, };
		::OVERLAPPED mSendOverlapped{ 0, };


		// 이미 보낸 버퍼정보를 들고 있음.
		SendingInfo mSendingInfo;

		// NOTE(pyoung): 나중에 RecvRingBuf<LinkedList> lockfree와 같이 버퍼를 공유하지 않는다면 성능 향상이 있을것인데...
		buffer::ringbuffer::RingBuffer mRecvQueue{ define::NET_SESSION_RECVQUEUE_SIZE };

		system::collections::concurrent::LockFreeQueue<common::net::Packet*> mSendQueue{ define::NET_SESSION_SENDQUEUE_SIZE };
		volatile uint32_t mIsCloseSocket{ 1 };
		uint32_t mRecvTime{ timeGetTime() };
		uint32_t mRecvSecondTick{ 0 };
		uint64_t mPrevUID{ 0 };
		int64_t mSessionUIDAcc{ 0 };

		nf::logging::Logger* mLogger;
	};
} // namespace nf::network::server::net::manager::session