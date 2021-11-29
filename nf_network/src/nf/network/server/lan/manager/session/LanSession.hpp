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
#include "nf/network/server/share/ReleaseInfo.hpp"
#include "nf/network/server/share/SendingBuffer.hpp"
#include "nf/system/collections/concurrent/LockFreeQueue.hpp"
#include "nf/network/common/Types.hpp"
#include "../../../share/eSessionSocketState.hpp"
#include "SendingInfo.hpp"

using namespace nf::network::server::share;
using namespace nf::network::common::types;

namespace nf::network::server::lan::manager::session
{
	class LanSessionManager;

	class LanSession final
	{
	public:
		LanSession() = default;
		virtual ~LanSession();
	public:
		LanSession(LanSession const& session) = delete;
		LanSession& operator=(LanSession const& session) = delete;

	public:
		constexpr static size_t ByteToBit(size_t const byte)
		{
			return (byte * 8);
		}
		inline static session_id GenSessionID(uint32_t const index, uint64_t const uid)
		{
			assert(uid != 0);
			return static_cast<session_id>(static_cast<uint64_t>(index) << ByteToBit(6) | (uid & 0x0000FFFFFFFFFFFF));
		}

		inline static uint32_t SessionIndex(session_id const sid)
		{
			return static_cast<uint32_t>(static_cast<uint64_t>(sid) >> ByteToBit(6));
		}

		inline static uint64_t SessionUID(session_id const sid)
		{
			return (static_cast<uint64_t>(sid) & 0x0000FFFFFFFFFFFF);
		}

	public:
		// Init -> Enqueue -> ReadySending -> FinishSending -> TryRelease
		void Init(session_id const sessionID, SOCKET const sock, char const* const address, uint16_t const port);
		bool Enqueue(common::lan::Packet* const sendPacket);
		bool ReadySending(SendingBuffer* const sendingBuffer);
		void FinishSending();
		bool TryRelease();
	public:
		inline uint32_t Index() const
		{
			return LanSession::SessionIndex(mLanSessionID);
		}

		inline uint64_t UID() const
		{
			return LanSession::SessionUID(mLanSessionID);
		}

		inline SOCKET GetSocket() const
		{
			return mSocket;
		}

		inline session_id GetSessionID() const
		{
			return mLanSessionID;
		}

		inline uint32_t GetRecvTime() const
		{
			return mRecvTime;
		}

		inline buffer::ringbuffer::RingBuffer& GetRecvQueue()
		{
			return mRecvQueue;
		}

		inline void SetRecvTime(uint32_t const recvTime)
		{
			// TODO(pyoung)
			mRecvTime = recvTime;
		}

		// TODO(pyoung): remove bellow
		inline::OVERLAPPED& GetRecvOverlapped()
		{
			return mRecvOverlapped;
		}

		inline void ResetRecvOverlapped()
		{
			::memset(&mRecvOverlapped, 0, sizeof(::OVERLAPPED));
		}

		inline::OVERLAPPED& GetSendOverlapped()
		{
			return mSendOverlapped;
		}

		inline bool IsReleased()
		{
			return mReleaseInfo.IsReleasedAtomic == 1;
		}

		inline void CancelationIO()
		{
			if (::InterlockedCompareExchange(&mIsCloseSocket, 1, 0) == 0)
			{
				::closesocket(mSocket);
				mSocket = INVALID_SOCKET;
			}
		}

		inline bool IsClosed()
		{
			return mIsCloseSocket == 1;
		}
	protected:
		// NOTE(pyoung): ioCount는 friend인 LanSessionManager만 접근할 수 있도록 한다.
		inline int32_t IncrementIOCount()
		{
			//printf("io (inc): %d\n", mReleaseInfo.IOCountAtomic);
			return ::InterlockedIncrement(reinterpret_cast<volatile uint32_t*>(&mReleaseInfo.IOCountAtomic));
		}

		inline int32_t DecrementIOCount()
		{
			//printf("io (dec): %d\n", mReleaseInfo.IOCountAtomic);
			return ::InterlockedDecrement(reinterpret_cast<volatile uint32_t*>(&mReleaseInfo.IOCountAtomic));
		}

		void CloseSock()
		{
			::closesocket(mSocket);
		}
	private:
		friend LanSessionManager;
	private:
		void Release();
	private:
		session_id mLanSessionID;
		::SOCKET mSocket{ INVALID_SOCKET };
		std::string mAddress;
		uint16_t mPort;

		::OVERLAPPED mRecvOverlapped{ 0, };
		::OVERLAPPED mSendOverlapped{ 0, };

		ReleaseInfo mReleaseInfo;

		SendingInfo mSendingInfo;

		buffer::ringbuffer::RingBuffer mRecvQueue{ define::LAN_SESSION_RECVQUEUE_SIZE }; //RecvRingBuf<LinkedList> lockfree
		
		system::collections::concurrent::LockFreeQueue<common::lan::Packet*> mSendQueue{ define::LAN_SESSION_SENDQUEUE_SIZE };

		volatile uint32_t mIsCloseSocket{ 1 };
		uint32_t mRecvTime{ timeGetTime() };
		uint32_t mRecvSecondTick{ 0 };
	};
} // namespace nf::network::server::lan::manager::session