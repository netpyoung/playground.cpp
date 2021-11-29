#include "LanSession.hpp"

namespace nf::network::server::lan::manager::session
{
	LanSession::~LanSession()
	{
		Release();
	}

	void LanSession::Init(session_id const sessionID, SOCKET const sock, char const* const address, uint16_t const port)
	{
		assert(address != nullptr);
		assert(sessionID != session_id::INVALID);
		assert(IsReleased());
		assert(IsClosed());
		mLanSessionID = sessionID;

		mSocket = sock;
		mPort = port;
		mAddress = std::string(address);

		assert(mSendQueue.GetUsedSize() == 0);
		mSendQueue.Clear();
		mRecvQueue.ClearBuffer();

		mReleaseInfo.IOCountAtomic = 0;
		mReleaseInfo.IsReleasedAtomic = 0;
		mSendingInfo.IsSendingAtomic = 0;

		assert(mIsCloseSocket == 1);
		mIsCloseSocket = 0;
	}

	void LanSession::Release()
	{
		CancelationIO();

		FinishSending();
		while (!mSendQueue.IsEmpty())
		{
			common::lan::Packet* sendPacket = nullptr;
			if (mSendQueue.TryDequeue(&sendPacket))
			{
				assert(sendPacket != nullptr);
				common::lan::PacketPool::TryFreeSend(sendPacket);
			}
		}
		mLanSessionID = session_id::INVALID;
		mSendingInfo.PacketCount = 0;
		assert(mSendQueue.GetUsedSize() == 0);
	}

	bool LanSession::TryRelease()
	{
		alignas(16) ReleaseInfo nextReleaseInfo { 1, 0 };

		if (InterlockedCompareExchange64(
			reinterpret_cast<int64_t*>(&mReleaseInfo),
			*reinterpret_cast<int64_t*>(&nextReleaseInfo),
			0) != 0)
		{
			return false;
		}
		Release();
		return true;
	}

	bool LanSession::Enqueue(common::lan::Packet* const sendPacket)
	{
		if (mReleaseInfo.IsReleasedAtomic == 1)
		{
			return false;
		}

		sendPacket->AddRef();
		bool const isEnqueue = mSendQueue.Enqueue(sendPacket);
		if (!isEnqueue)
		{
			common::lan::PacketPool::TryFreeSend(sendPacket);
		}
		return isEnqueue;
	}

	bool LanSession::ReadySending(SendingBuffer* const outSendingBuffer)
	{
		if (mSendingInfo.IsSendingAtomic != 0)
		{
			return false;
		}

		if (InterlockedIncrement64(&mSendingInfo.IsSendingAtomic) != 1)
		{
			InterlockedDecrement64(&mSendingInfo.IsSendingAtomic);
			return false;
		}

		size_t const queueSize = mSendQueue.GetUsedSize();
		if (queueSize > outSendingBuffer->Buffers.size())
		{
			throw std::invalid_argument("size must be same or lower than SendQueue's GetUsedSize");
		}

		if (queueSize == 0)
		{
			// looop continue.
			InterlockedDecrement64(&mSendingInfo.IsSendingAtomic);
			return false;
		}

		mSendingInfo.PacketCount = 0;
		mSendingInfo.TotalByteSize = 0;
		for (size_t i = 0; i < queueSize; ++i)
		{
			common::lan::Packet* packet = nullptr;
			bool const isDequeue = mSendQueue.TryDequeue(&packet);
			if (!isDequeue)
			{
				break;
			}
			assert(isDequeue);
			assert(packet != nullptr);

			mSendingInfo.PacketBuffer[mSendingInfo.PacketCount] = packet;
			mSendingInfo.PacketCount++;
			mSendingInfo.TotalByteSize += packet->GetPacketSize();

			outSendingBuffer->Buffers[i].buf = reinterpret_cast<char*>(packet->GetHeaderPtr());
			outSendingBuffer->Buffers[i].len = static_cast<uint32_t>(packet->GetPacketSize());
		}

		::memset(&mSendOverlapped, 0, sizeof(::OVERLAPPED));

		outSendingBuffer->BufferSize = static_cast<uint32_t>(queueSize);
		outSendingBuffer->Overlapped = &mSendOverlapped;
		return true;
	}

	void LanSession::FinishSending()
	{
		for (int32_t i = 0; i < mSendingInfo.PacketCount; ++i)
		{
			common::lan::Packet* const sendPacket = mSendingInfo.PacketBuffer[i];
			common::lan::PacketPool::TryFreeSend(sendPacket); // Main::OnReceived
		}
		mSendingInfo.PacketCount = 0;
		mSendingInfo.TotalByteSize = 0;
		InterlockedDecrement64(&mSendingInfo.IsSendingAtomic);
	}
} // namespace nf::network::server::lan::manager::session
