#include "SectorManager.hpp"

namespace nf::network::server::net::manager::sector
{
	SectorManager::SectorManager(int32_t const width, int32_t const height) :
		mWidth(width),
		mHeight(height),
		mSector(std::make_unique<std::list<ISectorUser*>[]>(static_cast<size_t>(width)* static_cast<size_t>(height)))
	{
	}

	int2 SectorManager::GetNowSectorPosFromSectorUser(ISectorUser* const user) const
	{
		return int2(user->GetX() / mWidth, user->GetY() / mHeight);
	}

	void SectorManager::AddSectorUser(ISectorUser* const user)
	{
		int2 const nowSectorPos = GetNowSectorPosFromSectorUser(user);
		std::list<ISectorUser*>& users = GetSectorUsersInSector(nowSectorPos);
		assert(std::find(users.begin(), users.end(), user) == users.end());
		users.push_back(user);

		user->SetCurSectorPos(nowSectorPos);
	}

	void SectorManager::DelSectorUser(ISectorUser* const user)
	{
		int2 const curSectorPos = user->GetCurSectorPos();
		std::list<ISectorUser*>& users = GetSectorUsersInSector(curSectorPos);
		auto it = std::find(users.begin(), users.end(), user);
		assert(it != users.end());
		users.erase(it);

		user->SetOldSectorPos(curSectorPos);
	}

	bool SectorManager::UpdateSectorUser(ISectorUser* const user)
	{
		int2 const nowSectorPos = GetNowSectorPosFromSectorUser(user);
		int2 const curSectorPos = user->GetCurSectorPos();
		if (nowSectorPos == curSectorPos)
		{
			return false;
		}
		DelSectorUser(user);
		AddSectorUser(user);
		return true;
	}

	void SectorManager::GetSectorAround(int32_t const sectorX, int32_t const sectorY, SectorAround* const sectorAround) const
	{
		for (int32_t y = sectorY - 1; y <= sectorY + 1; ++y)
		{
			if (0 <= y && y < mHeight)
			{
				for (int32_t x = sectorX - 1; x <= sectorX + 1; ++x)
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

	void SectorManager::GetUpdateSectorAround(ISectorUser* const user, SectorAround* const willRemoveSector, SectorAround* const willAddSector) const
	{
		willRemoveSector->Count = 0;
		willAddSector->Count = 0;

		SectorAround oldSectorAround;
		SectorAround curSectorAround;
		GetSectorAround(user->GetOldSectorPos().X, user->GetOldSectorPos().Y, &oldSectorAround);
		GetSectorAround(user->GetCurSectorPos().X, user->GetCurSectorPos().Y, &curSectorAround);
		for (int32_t oldCount = 0; oldCount < oldSectorAround.Count; ++oldCount)
		{
			bool isFound = false;
			for (int32_t curCount = 0; curCount < curSectorAround.Count; ++curCount)
			{
				if (oldSectorAround.Around[oldCount] == curSectorAround.Around[curCount])
				{
					isFound = true;

					break;
				}
			}
			if (!isFound)
			{
				willRemoveSector->Around[willRemoveSector->Count] = oldSectorAround.Around[oldCount];
				willRemoveSector->Count++;
			}
		}


		for (int32_t curCount = 0; curCount < curSectorAround.Count; ++curCount)
		{
			bool isFound = false;
			for (int32_t oldCount = 0; oldCount < oldSectorAround.Count; ++oldCount)
			{
				if (oldSectorAround.Around[oldCount] == curSectorAround.Around[curCount])
				{
					isFound = true;
					break;
				}
			}
			if (!isFound)
			{
				willAddSector->Around[willAddSector->Count] = curSectorAround.Around[curCount];
				willAddSector->Count++;
			}
		}
	}

	std::unordered_set<ISectorUser*> SectorManager::GetAroundSectorUsers(ISectorUser* const user)
	{
		std::unordered_set<ISectorUser*> ret;
		int2 const curSectorPos = user->GetCurSectorPos();
		SectorAround sectorAround;
		GetSectorAround(curSectorPos.X, curSectorPos.Y, &sectorAround);
		for (int32_t i = 0; i < sectorAround.Count; ++i)
		{
			for (auto* const sectorUser : GetSectorUsersInSector(sectorAround.Around[i]))
			{
				ret.insert(sectorUser);
			}
		}
		ret.erase(user);
		return ret;
	}

	void SectorManager::SendAroundExcept(ISectorUser* const user, Packet* const sendPacket)
	{
		int2 const curSectorPos = user->GetCurSectorPos();
		SectorAround sectorAround;
		GetSectorAround(curSectorPos.X, curSectorPos.Y, &sectorAround);

		for (int32_t i = 0; i < sectorAround.Count; ++i)
		{
			for (auto* const sectorUser : GetSectorUsersInSector(sectorAround.Around[i]))
			{
				assert(sectorUser != nullptr);

				if (sectorUser == user)
				{
					continue;
				}
				sendPacket->AddRef();
				sectorUser->Send(sendPacket);
			}
		}
	}

	void SectorManager::SendAround(int2 const sectorPos, Packet* const sendPacket)
	{
		SectorAround sectorAround;
		GetSectorAround(sectorPos.X, sectorPos.Y, &sectorAround);
		for (int32_t i = 0; i < sectorAround.Count; ++i)
		{
			auto sectorUsers = GetSectorUsersInSector(sectorAround.Around[i]);
			sendPacket->AddRef(sectorUsers.size());

			for (auto* const sectorUser : sectorUsers)
			{
				assert(sectorUser != nullptr);
				sectorUser->Send(sendPacket);
			}
		}
	}
} // namespace nf::network::server::net::manager::sector
