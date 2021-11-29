#pragma once

#pragma warning(push, 0)
#include <cstddef>
#include <cstdint>
#include <string>
#include <cassert>
#include <stdexcept>
#pragma warning(pop)

#include "LanSession.hpp"
#include "nf/system/collections/concurrent/LockFreeStack.hpp"
#include "nf/network/common/Types.hpp"

using namespace nf::network::common::types;

namespace nf::network::server::lan::manager::session
{
	class LanSessionManager final
	{
	public:
		LanSessionManager() = default;
		virtual ~LanSessionManager();
	public:
		void Init(uint32_t const size);

		LanSession* FindSessionOrNull(session_id const sessionID);

		//===================================
		// New Delete
		//===================================
		LanSession* NewSessionOrNull(::SOCKET const socket, char const* const address, uint16_t const port);
		bool TryReleaseSession(LanSession* const session);
		// ::shutdown(session->GetSocket(), SD_BOTH);
	public:
		inline int64_t IncrementIO(LanSession* const session)
		{
			return session->IncrementIOCount();
		}

		inline int64_t DecrementIO(LanSession* const session)
		{
			int64_t const decrement = session->DecrementIOCount();
			assert(decrement >= 0);
			return decrement;
		}

		inline int64_t ConnectedSessionCount() const
		{
			return mConnectedSessionCount;
		}

	private:
		void Cleanup();
		session_id GenUniqueSessionID();
	private:
		int64_t mConnectedSessionCount{ 0 };

		// session assign
		size_t mPoolSize{ 0 };
		std::unique_ptr<LanSession* []> mSessionPool;
		system::collections::concurrent::LockFreeStack<uint32_t> mUnusedSessionIndexes;
		uint32_t mSessionUIDAcc{ 0 };
	};
} // namespace nf::network::server::lan::manager::session
