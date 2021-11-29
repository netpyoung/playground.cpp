#include "LanSessionManager.hpp"

namespace nf::network::server::lan::manager::session
{
	LanSessionManager::~LanSessionManager()
	{
		Cleanup();
	}

	void LanSessionManager::Init(uint32_t const size)
	{
		Cleanup();
		
		mSessionPool = std::make_unique<LanSession* []>(size);
		mUnusedSessionIndexes.Init(size);
		mPoolSize = size;

		for (uint32_t i = 0; i < size; ++i)
		{
			mSessionPool[i] = new LanSession(); // Cleanup
		}
		for (uint32_t i = 0; i < size; ++i)
		{
			mUnusedSessionIndexes.Push(i);
		}
	}

	LanSession* LanSessionManager::FindSessionOrNull(session_id const sessionID)
	{
		uint32_t const index = LanSession::SessionIndex(sessionID);
		assert(index < mPoolSize);
		
		LanSession* const session = mSessionPool[index];
		assert(session != nullptr);
		
		return session;
	}

	//===================================
	// New Delete
	//===================================

	LanSession* LanSessionManager::NewSessionOrNull(::SOCKET const sessionSocket, char const* const address, uint16_t const port)
	{
		// NOTE(pyoung): only `ServerComponent::CreateLanSessionOrNull`

		assert(sessionSocket != INVALID_SOCKET);

		session_id const sessionID = GenUniqueSessionID();
		assert(sessionID != session_id::INVALID);

		LanSession* const session = mSessionPool[LanSession::SessionIndex(sessionID)];
		assert(session != nullptr);
		assert(session->IsReleased());

		session->Init(sessionID, sessionSocket, address, port);

		InterlockedIncrement64(&mConnectedSessionCount);
		return session;
	}

	bool LanSessionManager::TryReleaseSession(LanSession* const session)
	{
		// NOTE(pyoung): only `ServerComponent::RemoveLanSession`
		assert(session != nullptr);
		uint32_t const index = session->Index();

		bool const isSuccessRelease = session->TryRelease();
		if (isSuccessRelease)
		{
			if (!mUnusedSessionIndexes.Push(index))
			{
				throw std::logic_error("Unable to reuse sesion index");
			}

			int64_t const joinedUserCout = InterlockedDecrement64(&mConnectedSessionCount);
			assert(joinedUserCout >= 0);
		}
		return isSuccessRelease;
	}

	//===================================
	// Private
	//===================================
	void LanSessionManager::Cleanup()
	{
		for (size_t i = 0; i < mPoolSize; ++i)
		{
			delete(mSessionPool[i]); // Init
		}
		mSessionPool = nullptr;
		mPoolSize = 0;
		mUnusedSessionIndexes.Clear();
	}

	session_id LanSessionManager::GenUniqueSessionID()
	{
		uint32_t index = 0;
		bool const isPop = mUnusedSessionIndexes.TryPop(&index);
		assert(isPop);

		uint64_t uid = 0;
		while (true)
		{
			uid = InterlockedIncrement(&mSessionUIDAcc) & 0x0000FFFFFFFFFFFF;
			if (uid != 0 && uid != mSessionPool[index]->UID())
			{
				break;
			}
		}
		return LanSession::GenSessionID(index, uid);
	}

} // namespace nf::network::server::lan::manager::session
