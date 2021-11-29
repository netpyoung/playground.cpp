#pragma once

#include <memory>
#include <nf/system/threading/ManualResetEvent.hpp>
#include <nf/system/collections/concurrent/LockFreeQueue.hpp>
#include <nf/mathematics/int2.hpp>
#include <nf/network/server/net/NetCore.hpp>
#include <nf/network/server/net/manager/sector/SectorAround.hpp>

#include "eJobType.hpp"
#include "packet/Packet.hpp"
#include "Job.hpp"
#include "ChatUser.hpp"
#include "ChatUserManager.hpp"

using namespace nf;
using namespace nf::mathematics;
using namespace nf::network::server::net::manager::sector;

namespace chat
{
	class SectorManager final
	{
	public:
		SectorManager(int32_t const width, int32_t const height) :
			mWidth(width),
			mHeight(height),
			mSector(std::make_unique<std::list<ChatUser*>[]>(static_cast<size_t>(width)* static_cast<size_t>(height)))

		{
		}

		inline std::list<ChatUser*>& GetSectorUsersInSector(int2 pos)
		{
			assert(0 <= pos.X);
			assert(0 <= pos.X);
			assert(pos.X < mHeight);
			assert(pos.Y < mWidth);
			return mSector[static_cast<size_t>(pos.Y * mWidth + pos.X)];
		}

		void InsertSectorUser(ChatUser* const user, int2 const nowSectorPos)
		{
			std::list<ChatUser*>& users = GetSectorUsersInSector(nowSectorPos);
			assert(std::find(users.begin(), users.end(), user) == users.end());
			users.push_back(user);
			user->SetCurSectorPos(nowSectorPos);
		}

		void RemoveSectorUser(ChatUser* const user)
		{
			if (user == nullptr)
			{
				return;
			}

			int2 const curSectorPos = user->GetCurSectorPos();
			if (curSectorPos == mInvalidSectorPos)
			{
				return;
			}

			std::list<ChatUser*>& curSectorUsers = GetSectorUsersInSector(curSectorPos);
			auto it = std::find(curSectorUsers.begin(), curSectorUsers.end(), user);
			assert(it != curSectorUsers.end());
			curSectorUsers.erase(it);
		}

		void GetSectorAround(int2 sectorPos, SectorAround* const sectorAround) const
		{
			for (int32_t y = sectorPos.Y - 1; y <= sectorPos.Y + 1; ++y)
			{
				if (0 <= y && y < mHeight)
				{
					for (int32_t x = sectorPos.X - 1; x <= sectorPos.X + 1; ++x)
					{
						if (0 <= x && x < mWidth)
						{
							sectorAround->Around[sectorAround->Count].X = x;
							sectorAround->Around[sectorAround->Count].Y = y;
							sectorAround->Count++;
						}
					}
				}
			}
		}

		void SendAround(int2 const sectorPos, network::common::net::Packet* const sendPacket)
		{
			SectorAround curSectorAround;
			GetSectorAround(sectorPos, &curSectorAround);

			for (int32_t i = 0; i < curSectorAround.Count; ++i)
			{
				for (auto* const sectorUser : GetSectorUsersInSector(curSectorAround.Around[i]))
				{
					sectorUser->Send(sendPacket);
				}
			}
		}

		bool TryMove(ChatUser* const user, int2 const destSectorPos)
		{
			int2 const curSectorPos = user->GetCurSectorPos();
			if (curSectorPos == destSectorPos)
			{
				return false;
			}

			if (curSectorPos != mInvalidSectorPos)
			{
				RemoveSectorUser(user);
			}
			
			InsertSectorUser(user, destSectorPos);

			return true;
		}

	private:
		int32_t mWidth;
		int32_t mHeight;
		int2 mInvalidSectorPos{ -1, -1 };
		std::unique_ptr<std::list<ChatUser*>[]> mSector;
	};
} // namespace chat