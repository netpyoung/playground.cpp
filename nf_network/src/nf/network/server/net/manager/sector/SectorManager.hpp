#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
//#include <WinSock2.h>
#include <string>
#include <iostream>
#include <cassert>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <memory>
#pragma warning(pop)

#include "SectorAround.hpp"
#include "nf/mathematics/int2.hpp"
#include "../session/NetSession.hpp"
#include "../../../../common/net/Packet.hpp"

using namespace nf::mathematics;
using namespace nf::network::common::net;

namespace nf::network::server::net::manager::sector
{
	class ISectorUser
	{
	public:
		virtual ~ISectorUser() {}
		virtual int32_t GetX() const = 0;
		virtual int32_t GetY() const = 0;

		virtual void SetOldSectorPos(int2 const pos) = 0;
		virtual void SetCurSectorPos(int2 const pos) = 0;
		virtual int2 GetOldSectorPos() const = 0;
		virtual int2 GetCurSectorPos() const = 0;
		
		virtual void Send(Packet* const p) = 0;
	};

	class SectorManager final
	{
	public:
		SectorManager(int32_t const width, int32_t const height);
		SectorManager(SectorManager const& other) = delete;
		virtual ~SectorManager() = default;
		SectorManager& operator=(SectorManager const& other) = delete;
	public:
		int2 GetNowSectorPosFromSectorUser(ISectorUser* const user) const;
		void AddSectorUser(ISectorUser* const user);
		void DelSectorUser(ISectorUser* const user);
		bool UpdateSectorUser(ISectorUser* const user);
		void GetSectorAround(int32_t const sectorX, int32_t const sectorY, SectorAround* const sectorAround) const;
		void GetUpdateSectorAround(ISectorUser* const user, SectorAround* const willRemoveSector, SectorAround* const willAddSector) const;
		std::unordered_set<ISectorUser*> GetAroundSectorUsers(ISectorUser* const user);
		void SendAroundExcept(ISectorUser* const user, Packet* const sendPacket);
		void SendAround(int2 const sectorPos, Packet* const sendPacket);
	public:
		inline std::list<ISectorUser*>& GetSectorUsersInSector(int2 pos)
		{
			assert(0 <= pos.X);
			assert(0 <= pos.X);
			assert(pos.X < mHeight);
			assert(pos.Y < mWidth);
			return mSector[static_cast<size_t>(pos.Y * mWidth + pos.X)];
		}
		//void Debug();
	private:
		int32_t mWidth;
		int32_t mHeight;
		std::unique_ptr<std::list<ISectorUser*>[]> mSector;
	};
} // namespace nf::network::server::net::manager::sector
