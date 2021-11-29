#include <nf/os/windows/input/Keyboard.hpp>

#include "nf/network/server/lan/LanCore.hpp"

using namespace nf;


class EventHandler : protected network::server::lan::ILanCoreEvent
{
public:
	EventHandler(int32_t const a)
	{
		std::cout << a << std::endl;
	}
	virtual ~EventHandler()
	{
		printf("Delete");
	}
public:
	virtual void OnInitialized(network::server::lan::NetController const& serverSender) override
	{
		mSender = serverSender;
	}

	virtual void OnConnected(SessionInfo const& sessionInfo) override
	{

	}
	virtual void OnDisconnected(session_id const sessionID)  override
	{

	}
	virtual void OnError(ErrorInfo const& errorInfo)  override
	{

	}
	virtual void OnReceived(session_id const sessionID, network::common::lan::Packet* const recvPacket)  override
	{
		uint64_t val;
		*recvPacket >> &val;

		{
			network::common::lan::Packet* const sendPacket = network::common::lan::PacketPool::AllocSend(); // Session::FinishSending
			*sendPacket << val;
			assert(sendPacket->GetBodySize() == sizeof(uint64_t));

			mSender.SendPacket(sessionID, sendPacket);
			network::common::lan::PacketPool::TryFreeSend(sendPacket);
		}
	}
	virtual void OnQuit() override
	{
		mLogger.Debug("OnQuit", "Quit");
	}
	virtual void OnWorkerThreadStarted(int32_t const networkIOThreadFuncID)  override
	{
		//mLogger.Debug("OnWorkerThreadStarted", "%d", networkIOThreadFuncID);
	}
	virtual void OnWorkerThreadEnded(int32_t const networkIOThreadFuncID)  override
	{
		//mLogger.Debug("OnWorkerThreadEnded", "%d", networkIOThreadFuncID);
	}
	virtual void OnUpdate(ServerTimeInfo const& timeInfo)
	{
	}
private:
	network::server::lan::NetController mSender;
	logging::Logger mLogger{ logging::eLogLevel::Debug };
};

class App final
{
public:
	App() = default;
	virtual ~App()
	{
		mServerComponent = nullptr;
		mLogger = nullptr;
	}
public:
	int32_t Run(char const* const address, uint16_t const port)
	{
		network::server::share::ServerComponentOption option;
		option.Address = address;
		option.Port = port;
		option.IsEnableNagle = true;
		option.MaxPacketBodyLength = UINT16_MAX;
		option.NetworkIOThreadCount = 2u;

		mServerComponent->RegisterEventHandler<EventHandler>(1);

		// FIXME(pyoung): 이 아래 로직들을 Component안으로 넣을것.
		std::optional<eServerComponentError> const error = mServerComponent->Start(option);
		if (error)
		{
			mLogger->Error("main", "%d", error.value());
			return 1;
		}

		int32_t currMs = ::timeGetTime();
		int32_t prevMs;

		while (mServerComponent->IsRunning())
		{
			// TODO(pyoung): key랑 이벤트를 등록할 수 있는 장치 마련할것.
			mKeyboard.UpdateKey(os::windows::input::eKey::Esc);
			mKeyboard.UpdateKey(os::windows::input::eKey::P);

			if (mKeyboard.KeyDown(os::windows::input::eKey::Esc))
			{
				mServerComponent->Stop();
				break;
			}
			if (mKeyboard.KeyDown(os::windows::input::eKey::P))
			{
				mServerComponent->Profile();
			}


			prevMs = currMs;
			currMs = ::timeGetTime();
			int32_t const delta = currMs - prevMs;
			// TODO(pyoung): handle createdSocket
			// mServerComponent.Tick();
			// mServerComponent.GetTicker(); => // { deltaTime, upTime }
			mServerComponent->Tick(delta);
			Sleep(16);

		}
		return 0;
	}
private:
	std::unique_ptr<logging::Logger> mLogger{ std::make_unique<logging::Logger>(logging::eLogLevel::Debug) };
	std::unique_ptr<network::server::lan::LanCore> mServerComponent{ std::make_unique<network::server::lan::LanCore>(mLogger.get()) };
	os::windows::input::Keyboard mKeyboard;
};

int main()
{
	return App{}.Run("0.0.0.0", 6000u);
	//std::cout << sizeof(::DWORD) << std::endl;
	//std::cout << sizeof(::size_t) << std::endl;
	//::DWORD a = 1000;
	//size_t b = static_cast<size_t>(a);
	//std::cout << a << std::endl;
	//std::cout << b << std::endl;
	//return 0;
}
