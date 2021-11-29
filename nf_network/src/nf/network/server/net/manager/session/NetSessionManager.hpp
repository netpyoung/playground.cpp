#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
#include <string>
#include <cassert>
#include <stdexcept>
#pragma warning(pop)

#include "NetSession.hpp"
#include "nf/system/collections/concurrent/LockFreeStack.hpp"
#include "nf/logging/Logger.hpp"

namespace nf::network::server::net::manager::session
{
	class NetSessionManager final
	{
	public:
		NetSessionManager() = default;
		virtual ~NetSessionManager();
	public:
		void Init(uint32_t const size, nf::logging::Logger* const logger);

		NetSession* FindSessionOrNull(session_id const sessionID) const;

		//===================================
		// New Delete
		//===================================
		NetSession* NewSessionOrNull(::SOCKET const socket, char const* const address, uint16_t const port);
		bool TryReleaseSession(NetSession* const session);
	public:
		inline int64_t IncrementIO(NetSession* const session)
		{
			return session->IncrementIOCount();
		}

		inline int64_t DecrementIO(NetSession* const session)
		{
			int64_t const decremented = session->DecrementIOCount();
			if (decremented < 0)
			{
				mLogger->Error("DecrementIO", "decremented : %lld", decremented);
				throw std::logic_error("decremented should be possitive");
			}
			return decremented;
		}

		inline int64_t ConnectedSessionCount() const
		{
			return mConnectedSessionCount;
		}

		inline size_t GetCachedSessionCount() const
		{
			return mUnusedSession.GetUsedSize();
		}

		inline size_t GetSessionMaxCapacity() const
		{
			return mUnusedSession.GetMaxCapacity();
		}

	private:
		void Cleanup();
	private:
		int64_t mConnectedSessionCount{ 0 };

		// session assign
		size_t mPoolSize{ 0 };
		std::unique_ptr<NetSession* []> mSessionPool;
		system::collections::concurrent::LockFreeStack<NetSession*> mUnusedSession;
		nf::logging::Logger* mLogger{ nullptr };
	};
} // namespace nf::network::server::net::manager::session
