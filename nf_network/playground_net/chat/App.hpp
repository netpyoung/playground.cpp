#pragma once

#include <nf/os/windows/input/Keyboard.hpp>
#include <nf/system/threading/ManualResetEvent.hpp>
#include <nf/system/collections/concurrent/LockFreeQueue.hpp>
#include <nf/mathematics/int2.hpp>
#include <nf/network/server/net/NetCore.hpp>
#include <nf/network/server/net/manager/sector/SectorManager.hpp>

#include "EventHandler.hpp"

using namespace nf;
using namespace nf::mathematics;

namespace chat
{
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

#if _DEBUG
			std::cout << "_DEBUG : yes" << std::endl;
#else
			std::cout << "_DEBUG : no" << std::endl;
#endif
			static_assert(sizeof(network::common::net::Header) == 5);

			network::server::share::ServerComponentOption option;
			option.Address = address;
			option.Port = port;
			option.IsEnableNagle = true;
			option.MaxPacketBodyLength = UINT16_MAX;
			option.PacketCode = 0x77u;
			option.PacketEncodeKey = 0x32u;
			option.NetworkIOThreadCount = 4u;
			option.MaxSessionCount = 17000;
			option.KeepAliveOption.IsOn = true;

			time_t     now = time(0);
			struct tm  tstruct;
			char       buf[80];
			tstruct = *localtime(&now);
			strftime(buf, sizeof(buf), "%Y-%m-%d.log", &tstruct);
			mLogger->SetFilePath(buf);
			mLogger->SetLogLevel(nf::logging::eLogLevel::Debug);
			mServerComponent->RegisterEventHandler<EventHandler>(mLogger.get());

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
				Sleep(2000);
			}
			return 0;
		}
	private:
		std::unique_ptr<logging::Logger> mLogger{ std::make_unique<logging::Logger>(logging::eLogLevel::Trace) };
		std::unique_ptr<network::server::net::NetCore> mServerComponent{ std::make_unique<network::server::net::NetCore>(mLogger.get()) };
		os::windows::input::Keyboard mKeyboard;
	};

} // namespace chat