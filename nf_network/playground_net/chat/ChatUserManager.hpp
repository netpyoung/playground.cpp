#pragma once

#include <cstdint>
#include <nf/objectpool/ObjectPool.hpp>
#include "ChatUser.hpp"
#include "packet/Packet.hpp"

using namespace nf;

namespace chat
{
	class ChatUserManager final
	{
		using account_id = int64_t;
	public:
		ChatUserManager() = default;
		virtual ~ChatUserManager()
		{
			for (auto [sessionID, user] : mAccountUserMap)
			{
				mUserPool.Return(user);
			}
		}

	public:
		std::pair<bool, ChatUser*> AddUser(session_id const sessionID, packet::CS_CHAT_REQ_LOGIN const& q)
		{
			if (mSessionUserMap.find(sessionID) != mSessionUserMap.end())
			{
				return { false, nullptr };
			}
			if (mAccountUserMap.find(q.AccountNo) != mAccountUserMap.end())
			{
				return { false, nullptr };
			}

			ChatUser* const user = mUserPool.Rent(sessionID, q);
			mAccountUserMap.insert({ user->AccountNo, user });
			mSessionUserMap.insert({ sessionID, user });
			user->RegisterSender(mSeverSender);
			return { true, user };
		}

		void RemoveUser(session_id const sessionID)
		{
			auto const sit = mSessionUserMap.find(sessionID);
			if (sit == mSessionUserMap.end())
			{
				throw std::logic_error("mSessionUserMap");
			}

			ChatUser* const user = sit->second;
			account_id const accountID = user->AccountNo;

			auto ait = mAccountUserMap.find(accountID);
			if (ait == mAccountUserMap.cend())
			{
				throw std::logic_error("mAccountUserMap");
			}

			mSessionUserMap.erase(sit);
			mAccountUserMap.erase(ait);

			mUserPool.Return(user);
		}

		ChatUser* FindUserOrNull(session_id const sessionID)
		{
			auto it = mSessionUserMap.find(sessionID);
			if (it == mSessionUserMap.end())
			{
				return nullptr;
			}
			return it->second;
		}

		std::unordered_map<session_id, ChatUser*>& GetSesionUserMap()
		{
			return mSessionUserMap;
		}

		void RegisterSender(network::server::net::NetController const severSender)
		{
			mSeverSender = severSender;
		}
	private:

		struct UserHasher
		{
			std::size_t operator()(const ChatUser* const user) const
			{
				return static_cast<size_t>(user->AccountNo);
			}
		};

		std::unordered_map<session_id, ChatUser*> mSessionUserMap;
		std::unordered_map<account_id, ChatUser*> mAccountUserMap;
		objectpool::ObjectPool<ChatUser, nf::memory::eAllocationPolicy::CallConstructorDestructor> mUserPool{ 1024 };
		network::server::net::NetController mSeverSender;
	};
}
