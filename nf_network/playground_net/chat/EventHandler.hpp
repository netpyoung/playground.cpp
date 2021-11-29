#pragma once

#include <memory>
#include <nf/system/threading/ManualResetEvent.hpp>
#include <nf/system/collections/concurrent/LockFreeQueue.hpp>
#include <nf/mathematics/int2.hpp>
#include <nf/network/server/net/NetCore.hpp>
#include <nf/network/server/net/manager/sector/SectorManager.hpp>

#include "eJobType.hpp"
#include "packet/Packet.hpp"
#include "Job.hpp"
#include "ChatUser.hpp"
#include "ChatUserManager.hpp"
#include "SectorManager.hpp"

using namespace nf;
using namespace nf::mathematics;

namespace chat
{
	constexpr int32_t dfSECTOR_X_MAX = 50;
	constexpr int32_t dfSECTOR_Y_MAX = 50;

	eJobType GetJobTypeFromPacketType(packet::ePacketType const packetType)
	{
		switch (packetType)
		{
		case packet::ePacketType::CS_CHAT_REQ_LOGIN:		return eJobType::UserLogin;
		case packet::ePacketType::CS_CHAT_REQ_SECTOR_MOVE:	return eJobType::UserSectorMove;
		case packet::ePacketType::CS_CHAT_REQ_MESSAGE:		return eJobType::UserMessage;
		case packet::ePacketType::CS_CHAT_REQ_HEARTBEAT:	return eJobType::UserHeartBeat;
		default:											return eJobType::INVALID;
		}
	}

	class EventHandler : protected network::server::net::INetCoreEvent
	{
	public:
		EventHandler(logging::Logger* const logger)
		{
			mLogger = logger;
		}
		virtual ~EventHandler() override
		{
		}
	public:
		virtual void OnInitialized(network::server::net::NetController const& serverSender) override
		{
			mSender = serverSender;
			mProcessJobThread = std::thread(&EventHandler::ThreadProcessJob, this);
			mHeartbeatThread = std::thread(&EventHandler::ThreadHeatbeatThread, this);
			mHandlerMonitorhread = std::thread(&EventHandler::ThreadHandlerMonitorhread, this);

			mUserManager->RegisterSender(mSender);
		}

		virtual void OnConnected(SessionInfo const& sessionInfo) override
		{
		}

		virtual void OnDisconnected(session_id const sessionID) override
		{
			mJobQueue.Enqueue(mJobPool.Rent(eJobType::SystemDisconnectUser, sessionID, nullptr));
			mJobEvent.Set();
		}

		virtual void OnError(ErrorInfo const& errorInfo) override
		{
			mLogger->Warn("OnError", "ErrorInfo [%d] %s", errorInfo.Error, errorInfo.Message.c_str());
		}

		virtual void OnReceived(session_id const sessionID, network::common::net::Packet* const recvPacket) override
		{
			if (mIsQuit)
			{
				return;
			}

			packet::ePacketType packetType;
			size_t const peekSize = recvPacket->PeekBody(reinterpret_cast<std::byte*>(&packetType), sizeof(packetType));
			if (peekSize != sizeof(packetType))
			{
				mLogger->Debug("OnReceived", "Counn't peek enough: %llu / %llu", peekSize, sizeof(packetType));
				mSender.Disconnect(sessionID); // OnReceived - (jobType == eJobType::INVALID)
				return;
			}

			eJobType jobType = GetJobTypeFromPacketType(packetType);
			if (jobType == eJobType::INVALID)
			{
				mLogger->Trace("OnReceived", "(jobType == eJobType::INVALID) : %u", packetType);
				mSender.Disconnect(sessionID); // OnReceived - (jobType == eJobType::INVALID)
				return;
			}

			int64_t const refCount = recvPacket->AddRef();
			if (refCount != 2)
			{
				network::common::net::PacketPool::TryFreeRecv(recvPacket);

				mLogger->Error("OnReceived", "recvPacket refCount != 2 Error");
				throw std::logic_error("recvPacket refCount != 2 Error!!");
			}

			mJobQueue.Enqueue(mJobPool.Rent(jobType, sessionID, recvPacket));
			mJobEvent.Set();
		}
		virtual void OnQuit() override
		{
			mIsQuit = true;
			mQuitEvent.Set();
			mJobEvent.Set();

			mProcessJobThread.join();
			mHeartbeatThread.join();
			mHandlerMonitorhread.join();

			{ // consume jobQueue
				while (!mJobQueue.IsEmpty())
				{
					Job* job;
					if (!mJobQueue.TryDequeue(&job))
					{
						continue;
					}
					if (job->JobType == eJobType::SystemDisconnectUser)
					{
						JobSystemDisconnectUser(job);
					}
					if (job->RecvPacket != nullptr)
					{
						network::common::net::PacketPool::TryFreeRecv(job->RecvPacket);
					}
					mJobPool.Return(job);
				}
			}
		}
		virtual void OnWorkerThreadStarted(int32_t const networkIOThreadFuncID) override
		{
			//mLogger.Debug("OnWorkerThreadStarted", "%d", networkIOThreadFuncID);
		}
		virtual void OnWorkerThreadEnded(int32_t const networkIOThreadFuncID) override
		{
			//mLogger.Debug("OnWorkerThreadEnded", "%d", networkIOThreadFuncID);
		}

		virtual void OnUpdate(ServerTimeInfo const& /*timeInfo*/) override
		{
		}
	public:

		void ThreadHeatbeatThread()
		{
			while (true)
			{
				if (mQuitEvent.WaitOne(60000))
				{
					break;
				}
				mJobQueue.Enqueue(mJobPool.Rent(eJobType::SystemHeartBeat, session_id::INVALID, nullptr));
				mJobEvent.Set();
			}
		}

		void ThreadHandlerMonitorhread()
		{
			while (true)
			{
				if (mQuitEvent.WaitOne(1000))
				{
					break;
				}
				std::cout
					<< "**  JobTPS **         : " << InterlockedExchange(&mJobTPS, 0) << std::endl
					<< "**  JobQueue **       : " << mJobQueue.GetUsedSize() << std::endl;
			}
		}

		void ThreadProcessJob()
		{
			while (true)
			{
				mJobEvent.WaitOne();
				if (mIsQuit)
				{
					return;
				}

				while (!mJobQueue.IsEmpty())
				{
					Job* job;
					if (!mJobQueue.TryDequeue(&job))
					{
						throw std::logic_error("Failed to dequeue");
					}

					bool isHandled = false;
					try
					{
						isHandled = HandleJob(job);
					}
					catch (nf::network::packet::PacketReaderException const& /*exPacketReader*/)
					{
						mLogger->Trace("ThreadProcessJob", "PacketReaderException [%llu]", job->SessionID);
					}
					catch (...)
					{
						mLogger->Error("ThreadProcessJob", "WTF catch");
					}

					if (!isHandled)
					{
						mSender.Disconnect(job->SessionID); // ThreadProcessJob - (!isHandled)
					}

					if (job->RecvPacket != nullptr)
					{
						network::common::net::PacketPool::TryFreeRecv(job->RecvPacket);
					}
					mJobPool.Return(job);
					::InterlockedIncrement(&mJobTPS);
				}
			}
		}
	public:
		bool HandleJob(Job* const job)
		{
			switch (job->JobType)
			{
			case eJobType::UserLogin:
				JobUserLogin(job);
				break;
			case eJobType::UserSectorMove:
				JobUserSectorMove(job);
				break;
			case eJobType::UserMessage:
				JobUserMessage(job);
				break;
			case eJobType::UserHeartBeat:
				JobUserHeartBeat(job);
				break;
			case eJobType::SystemDisconnectUser:
				JobSystemDisconnectUser(job);
				break;
			case eJobType::SystemHeartBeat:
				JobSystemHeartBeat(job);
				break;
			default:
				throw std::logic_error("wtf");
			}
			return true;
		}

		void JobUserLogin(Job* const job)
		{
			network::common::net::Packet* const recvPacket = job->RecvPacket;
			assert(recvPacket != nullptr);

			packet::CS_CHAT_REQ_LOGIN q;
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&q.Type), sizeof(q.Type));
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&q.AccountNo), sizeof(q.AccountNo));
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&q.ID), sizeof(q.ID));
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&q.Nickname), sizeof(q.Nickname));
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&q.SessionKey), sizeof(q.SessionKey));

			auto [isSuccess, user] = mUserManager->AddUser(job->SessionID, q);

			network::common::net::Packet* const sendPacket = network::common::net::PacketPool::AllocSend();
			{
				//uint16_t Type;
				//int8_t Status;	// 0:실패	1:성공
				//int64_t AccountNo;

				*sendPacket << static_cast<uint16_t>(packet::ePacketType::CS_CHAT_RES_LOGIN);
				if (isSuccess)
				{
					*sendPacket << static_cast<int8_t>(1);
					*sendPacket << user->AccountNo;
				}
				else
				{
					*sendPacket << static_cast<int8_t>(0);
					*sendPacket << q.AccountNo;
				}
				mSender.SendPacket(job->SessionID, sendPacket);
			}
			network::common::net::PacketPool::TryFreeSend(sendPacket);
		}

		void JobUserSectorMove(Job* const job)
		{
			network::common::net::Packet* const recvPacket = job->RecvPacket;
			assert(recvPacket != nullptr);

			packet::CS_CHAT_REQ_SECTOR_MOVE q;
			*recvPacket >> &q.Type;
			*recvPacket >> &q.AccountNo;
			*recvPacket >> &q.SectorX;
			*recvPacket >> &q.SectorY;

			// TODO(pyoung): 메시지 Get에러.

			ChatUser* const user = mUserManager->FindUserOrNull(job->SessionID);
			if (user == nullptr)
			{
				//mLogger->Debug("JobUserSectorMove", "Fail to FindUser [%llu]", job->SessionID);
				//mSender.Disconnect(job->SessionID);
				return;
			}

			if (user->AccountNo != q.AccountNo)
			{
				mLogger->Debug("JobUserSectorMove", "invalid AccountNo [%llu] user->AccountNo=%lld / q.AccountNo=%lld", job->SessionID, user->AccountNo, q.AccountNo);
				mSender.Disconnect(job->SessionID); // JobUserSectorMove : (user->AccountNo != q.AccountNo)
				return;
			}
			assert(user->AccountNo == q.AccountNo);

			network::common::net::Packet* const sendPacket = network::common::net::PacketPool::AllocSend();
			{
				packet::CS_CHAT_RES_SECTOR_MOVE r;
				r.Type = static_cast<uint16_t>(packet::ePacketType::CS_CHAT_RES_SECTOR_MOVE);
				r.AccountNo = user->AccountNo;
				r.SectorX = q.SectorX;
				r.SectorY = q.SectorY;

				*sendPacket << r.Type;
				*sendPacket << r.AccountNo;
				*sendPacket << r.SectorX;
				*sendPacket << r.SectorY;
				if (mSectorManager->TryMove(user, int2(r.SectorX, r.SectorY)))
				{
				}
				//mSectorManager->SendAround(int2(r.SectorX, r.SectorY), sendPacket);
				mSender.SendPacket(job->SessionID, sendPacket);
			}
			network::common::net::PacketPool::TryFreeSend(sendPacket);
		}

		void JobUserMessage(Job* const job)
		{
			//wchar_t wLastMessage[1] = { '=' };
			static_assert(sizeof(wchar_t) == 2);

			network::common::net::Packet* const recvPacket = job->RecvPacket;
			assert(recvPacket != nullptr);

			packet::CS_CHAT_REQ_MESSAGE q;
			*recvPacket >> &q.Type;
			*recvPacket >> &q.AccountNo;
			*recvPacket >> &q.MessageLen;
			recvPacket->GetBody(reinterpret_cast<std::byte*>(&mMessageBuffer), q.MessageLen);

			/*if (memcmp(reinterpret_cast<void*>(wLastMessage), reinterpret_cast<void*>(mMessageBuffer), 2) == 0)
			{
				mSender.MarkWillClose(job->SessionID);
			}*/

			// ref: https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
			//char messageBuffer[9999] = { 0 , };
			//int32_t const len = WideCharToMultiByte(CP_UTF8, 0, mMessageBuffer, lstrlenW(mMessageBuffer), NULL, 0, NULL, NULL);
			//WideCharToMultiByte(CP_UTF8, 0, mMessageBuffer, lstrlenW(mMessageBuffer), messageBuffer, len, NULL, NULL);
			//std::cout << messageBuffer << std::endl;

			ChatUser const* const user = mUserManager->FindUserOrNull(job->SessionID);
			if (user == nullptr)
			{
				mLogger->Debug("JobUserMessage", "Fail to FindUser [%llu]", job->SessionID);
				mSender.Disconnect(job->SessionID); // JobUserMessage (user == nullptr)
				return;
			}

			if (user->AccountNo != q.AccountNo)
			{
				mLogger->Trace("JobUserMessage", "invalid AccountNo [%llu] user->AccountNo=%lld / q.AccountNo=%lld", job->SessionID, user->AccountNo, q.AccountNo);
				mSender.Disconnect(job->SessionID); // JobUserMessage (user->AccountNo != q.AccountNo)
				return;
			}

			network::common::net::Packet* const sendPacket = network::common::net::PacketPool::AllocSend();
			{
				packet::CS_CHAT_RES_MESSAGE r;
				r.Type = static_cast<uint16_t>(packet::ePacketType::CS_CHAT_RES_MESSAGE);
				assert(q.AccountNo == user->AccountNo);
				r.AccountNo = q.AccountNo;
				r.MessageLen = q.MessageLen;

				//	uint16_t Type;
				//	int64_t AccountNo;
				//	WCHAR	ID[20];						// null 포함
				//	WCHAR	Nickname[20];				// null 포함
				//	uint16_t MessageLen;
				//	WCHAR	Message[MessageLen / 2]		// null 미포함
				*sendPacket << r.Type;
				*sendPacket << r.AccountNo;
				sendPacket->PutBody(reinterpret_cast<std::byte const*>(&user->ID), sizeof(user->ID));
				sendPacket->PutBody(reinterpret_cast<std::byte const*>(&user->Nickname), sizeof(user->Nickname));
				*sendPacket << r.MessageLen;
				sendPacket->PutBody(reinterpret_cast<std::byte*>(&mMessageBuffer), r.MessageLen);
				mSectorManager->SendAround(user->GetCurSectorPos(), sendPacket);
			}
			network::common::net::PacketPool::TryFreeSend(sendPacket);
		}

		void JobUserHeartBeat(Job* const job)
		{
			// recived heatbeat ping from user.

			// TODO(pyoung): update heartbeat for user;
			ChatUser* const user = mUserManager->FindUserOrNull(job->SessionID);
			if (user == nullptr)
			{
				return;
			}
			user->UpdateHeartBeat();
		}

		void JobSystemDisconnectUser(Job* const job)
		{
			ChatUser* const user = mUserManager->FindUserOrNull(job->SessionID);
			if (user == nullptr)
			{
				return;
			}

			//session_id const sessionID = job->SessionID;
			//uint32_t const sessionIndex = nf::network::server::net::manager::session::NetSession::SessionIndex(job->SessionID);
			//uint64_t const sessionUID = nf::network::server::net::manager::session::NetSession::SessionUID(job->SessionID);
			//mLogger->Debug("JobSystemDisconnectUser", "[%llu - %u | %llu] AccountNo : %lld", sessionID, sessionIndex, sessionUID, user->AccountNo);

			mSectorManager->RemoveSectorUser(user);
			mUserManager->RemoveUser(job->SessionID);
		}

		void JobSystemHeartBeat(Job* const job)
		{
			for (auto [sessionID, user] : mUserManager->GetSesionUserMap())
			{
				if (!user->IsHeartBeating())
				{
					mSender.Disconnect(sessionID); // JobSystemHeartBeat - (!user->IsHeartBeating())
				}
			}
			//std::cout << "mJobPool.GetUsedSize(): " << mJobPool.GetUsedSize() << std::endl;
		}

	private:
		wchar_t mMessageBuffer[1024]{ 0, }; // NOTE(pyoung): 채팅 메시지 버퍼는 일단 1kb로
		std::atomic_bool mIsQuit{ false };
		system::threading::AutoResetEvent mJobEvent{ false };
		system::threading::ManualResetEvent mQuitEvent;

		system::collections::concurrent::LockFreeQueue<Job*> mJobQueue{ 1000 };
		objectpool::ObjectPoolLockFree<Job, nf::memory::eAllocationPolicy::CallConstructorDestructor> mJobPool;
		network::server::net::NetController mSender;
		std::thread mProcessJobThread;
		std::thread mHeartbeatThread;
		std::thread mHandlerMonitorhread;

		volatile uint64_t mJobTPS{ 0 };
		logging::Logger* mLogger{ nullptr };
		std::unique_ptr<ChatUserManager> mUserManager{ std::make_unique<ChatUserManager>() };
		std::unique_ptr<SectorManager> mSectorManager{ std::make_unique<SectorManager>(dfSECTOR_X_MAX, dfSECTOR_Y_MAX) };
	};
} // namespace chat