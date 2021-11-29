#pragma once

#include <cstdint>
#include <nf/network/server/net/INetCoreEvent.hpp>
#include "packet/Packet.hpp"

namespace chat
{
	class ChatUser
	{
	public:
		ChatUser(session_id const sessionID, packet::CS_CHAT_REQ_LOGIN const& q)
		{
			mSessionID = sessionID;
			AccountNo = q.AccountNo;
			wcsncpy_s(ID, q.ID, 20);
			wcsncpy_s(Nickname, q.Nickname, 20);
			strncpy(SessionKey, q.SessionKey, 64); // 이부분 버그.
			mCurSectorPos = mInvalidSectorPos;
		}
		virtual ~ChatUser() = default;
	public:

		void Send(Packet* const p)
		{
			mSeverSender.SendPacket(mSessionID, p);
		}
	public:
		void UpdateHeartBeat()
		{

		}
		bool IsHeartBeating()
		{
			return true;
		}
	public:
		inline int2 GetCurSectorPos() const
		{
			return mCurSectorPos;
		}

		inline void SetCurSectorPos(int2 const newSectorPos)
		{
			mCurSectorPos = newSectorPos;
		}

		inline session_id GetSessionID() const
		{
			return mSessionID;
		}

		void RegisterSender(nf::network::server::net::NetController const severSender)
		{
			mSeverSender = severSender;
		}
	public:

		int64_t AccountNo;
		WCHAR ID[20];// null 포함
		WCHAR Nickname[20];// null 포함
		char SessionKey[64];
		int2 mInvalidSectorPos{ -1, -1 };

	private:
		session_id mSessionID;

		int2 mCurSectorPos;
		nf::network::server::net::NetController mSeverSender;
	};
} // namespace chat