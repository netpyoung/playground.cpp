#include "NetSessionManager.hpp"

namespace nf::network::server::net::manager::session
{
	NetSessionManager::~NetSessionManager()
	{
		Cleanup();
	}

	void NetSessionManager::Init(uint32_t const size, nf::logging::Logger* const logger)
	{
		Cleanup();

		mSessionPool = std::make_unique<NetSession* []>(size);
		mUnusedSession.Init(size);
		mPoolSize = size;

		for (uint32_t i = 0; i < size; ++i)
		{
			NetSession* const session = new NetSession(i, logger);
			mSessionPool[i] = session;
			mUnusedSession.Push(session);
		}
		mLogger = logger;
	}

	NetSession* NetSessionManager::FindSessionOrNull(session_id const sessionID) const
	{
		uint32_t const index = NetSession::SessionIndex(sessionID);
		assert(index < mPoolSize);

		NetSession* const session = mSessionPool[index];
		assert(session != nullptr);

		if (session->GetSessionID() != sessionID)
		{
			return nullptr;
		}

		if (session->IsClosed())
		{
			return nullptr;
		}

		if (session->IsReleased())
		{
			return nullptr;
		}

		return session;
	}

	//===================================
	// New Delete
	//===================================

	NetSession* NetSessionManager::NewSessionOrNull(::SOCKET const sessionSocket, char const* const address, uint16_t const port)
	{
		// NOTE(pyoung): only `ServerComponent::CreateSessionOrNull`

		assert(sessionSocket != INVALID_SOCKET);
		NetSession* session = nullptr;

		bool const isPop = mUnusedSession.TryPop(&session);
		if (!isPop)
		{
			return nullptr;
		}

		if (session == nullptr)
		{
			throw std::logic_error("WTF asshole");
		}

		session->Init(sessionSocket, address, port);
		InterlockedIncrement64(&mConnectedSessionCount);
		return session;
	}

	bool NetSessionManager::TryReleaseSession(NetSession* const session)
	{
		// NOTE(pyoung): only `ServerComponent::TryRemoveSession`
		assert(session != nullptr);

		if (session->IsReleased())
		{
			return false;
		}

		if (!session->TryRelease())
		{
			return false;
		}

		if (!mUnusedSession.Push(session))
		{
			throw std::logic_error("Unable to reuse sesion index");
		}

		int64_t const joinedUserCout = InterlockedDecrement64(&mConnectedSessionCount);
		assert(joinedUserCout >= 0);
		return true;
	}

	//===================================
	// Private
	//===================================
	void NetSessionManager::Cleanup()
	{
		for (size_t i = 0; i < mPoolSize; ++i)
		{
			delete(mSessionPool[i]); // Init
		}
		mSessionPool = nullptr;
		mPoolSize = 0;
		mUnusedSession.Clear();
	}
} // namespace nf::network::server::net::manager::session
