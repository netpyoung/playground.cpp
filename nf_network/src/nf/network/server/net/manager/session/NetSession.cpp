#include "NetSession.hpp"

namespace nf::network::server::net::manager::session
{
	NetSession::NetSession(uint32_t const sessionIndex, nf::logging::Logger* const logger)
	{
		mLogger = logger;
		mSessionIndex = sessionIndex;
	}

	NetSession::~NetSession()
	{
		Release();
	}

	void NetSession::Init(SOCKET const sock, char const* const address, uint16_t const port)
	{
		assert(sock != INVALID_SOCKET);
		assert(address != nullptr);

		assert(IsReleased());

		if (!IsReleased())
		{
			throw std::logic_error("Session is not released");
		}

		uint64_t const oldSessionUID = PrevUID();
		while (true)
		{
			mSessionUIDAcc++;
			uint64_t const newSessionUID = mSessionUIDAcc & 0x0000FFFFFFFFFFFF;
			if (newSessionUID != 0 && newSessionUID != oldSessionUID)
			{
				mNetSessionID = NetSession::GenSessionID(Index(), newSessionUID);
				break;
			}
		}
		mSocket = sock;
		mPort = port;
		mAddress = std::string(address);

		assert(mSendQueue.GetUsedSize() == 0);
		mSendQueue.Clear();
		mRecvQueue.ClearBuffer();

		//assert(mReleaseInfo.IsReleasedAtomic == 1);
		//assert(mReleaseInfo.IOCountAtomic == 0);
		//mReleaseInfo.IOCountAtomic = 0;

		assert(mSendingInfo.IsSendingAtomic == 0);
		mSendingInfo.Init();

		assert(mIsCloseSocket == 1);
		mIsCloseSocket = 0;
		mReleaseInfo.IsReleasedAtomic = 0;
	}

	void NetSession::Release()
	{
		//CancelationIO(); // NetSession::Release
		InterlockedExchange(&mIsCloseSocket, 1);
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;

		mPrevUID = UID();
		mNetSessionID = session_id::INVALID;

		while (!mSendQueue.IsEmpty())
		{
			common::net::Packet* sendPacket = nullptr;
			if (mSendQueue.TryDequeue(&sendPacket))
			{
				assert(sendPacket != nullptr);
				common::net::PacketPool::TryFreeSend(sendPacket);
			}
		}
		FinishSending();
	}

	bool NetSession::TryRelease()
	{
		alignas(16) ReleaseInfo const nextReleaseInfo { 1, 0 };

		if (InterlockedCompareExchange64(
			reinterpret_cast<int64_t*>(&mReleaseInfo),
			*reinterpret_cast<int64_t const*>(&nextReleaseInfo),
			0) != 0)
		{
			return false;
		}

		Release();

		// 릴리즈 후 index를 돌려주록 true를 반환.
		return true;
	}

	bool NetSession::Enqueue(common::net::Packet* const sendPacket)
	{
		if (!IsSendRecvable())
		{
			return false;
		}

		sendPacket->AddRef();
		bool const isEnqueue = mSendQueue.Enqueue(sendPacket);
		if (!isEnqueue)
		{
			common::net::PacketPool::TryFreeSend(sendPacket);
			mLogger->Debug("Enqueue", "mSendqueue is fulled");
			CancelationIO(); // NetSession::Enqueue (!isEnqueue)
		}
		return isEnqueue;
	}

	bool NetSession::ReadySending(SendingBuffer* const outSendingBuffer)
	{
		// LONG64 InterlockedCompareExchange64(
		// LONG64 volatile* Destination,
		//	LONG64          ExChange,
		//	LONG64          Comperand
		//	);
		// The function returns the initial value of the Destination parameter.

		int64_t const sendingState = InterlockedCompareExchange64(&mSendingInfo.IsSendingAtomic, 1, 0);
		if (sendingState != 0)
		{
			return false;
		}

		size_t const queueSize = mSendQueue.GetUsedSize();
		if (queueSize > outSendingBuffer->Buffers.size())
		{
			throw std::invalid_argument("size must be same or lower than SendQueue's GetUsedSize");
		}

		if (queueSize == 0)
		{
			// 이전 Send요청에서 queue를 다 쓴 것이므로 플레그만 변경시켜주자.
			InterlockedCompareExchange64(&mSendingInfo.IsSendingAtomic, 0, 1);
			return false;
		}

		// 이 작업중 소켓이 종료되면?
		mSendingInfo.PacketCount = 0;
		mSendingInfo.TotalByteSize = 0;
		size_t dequeuedSize = 0;
		for (size_t i = 0; i < queueSize; ++i)
		{
			common::net::Packet* packet = nullptr;
			bool const isDequeue = mSendQueue.TryDequeue(&packet);
			if (!isDequeue)
			{
				break;
			}

			if (packet == nullptr)
			{
				throw std::logic_error("Dequeued Packet is null");
			}

			mSendingInfo.PacketBuffer[mSendingInfo.PacketCount] = packet;
			mSendingInfo.PacketCount++;
			mSendingInfo.TotalByteSize += packet->GetPacketSize();

			outSendingBuffer->Buffers[i].buf = reinterpret_cast<char*>(packet->GetHeaderPtr());
			outSendingBuffer->Buffers[i].len = static_cast<uint32_t>(packet->GetPacketSize());
			dequeuedSize++;
		}

		if (mSendingInfo.PacketCount == 0)
		{
			InterlockedCompareExchange64(&mSendingInfo.IsSendingAtomic, 0, 1);
			return false;
		}

		outSendingBuffer->BufferSize = static_cast<uint32_t>(dequeuedSize);
		outSendingBuffer->Overlapped = &mSendOverlapped;

		InterlockedCompareExchange64(&mSendingInfo.IsSendingAtomic, 2, 1);
		return true;

	}

	int32_t NetSession::FinishSending()
	{
		// 센딩은 한번에 하나씩. (이 작업 중 패킷을 센딩할 수 없다)
		// FinishSending을 호출 조건
		// - 센딩을 완료하면 
		// - 세션을 해제할때
		int32_t const packetCount = mSendingInfo.PacketCount;
		for (int32_t i = 0; i < mSendingInfo.PacketCount; ++i)
		{
			common::net::Packet* const sendPacket = mSendingInfo.PacketBuffer[i];
			if (sendPacket == nullptr)
			{
				throw std::logic_error("mSendingInfo.PacketBuffer - sendPacket is null");
			}
			common::net::PacketPool::TryFreeSend(sendPacket); // Main::OnReceived
		}
		mSendingInfo.PacketCount = 0;
		mSendingInfo.TotalByteSize = 0;
		InterlockedCompareExchange64(&mSendingInfo.IsSendingAtomic, 0, 2);
		return packetCount;
	}

} // namespace nf::network::server::net::manager::session
