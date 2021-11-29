#pragma once

#include <cstdint>
#include <string>

namespace nf::network::server::share
{
	struct KeepAliveOption
	{
		bool IsOn{ true };
		::ULONG LiveMS{ 60 * 1000 }; // 60 sec
		::ULONG IntervalMS{ 10 * 1000 }; // 10sec
	};

	struct ServerComponentOption
	{
		std::string Address{ "127.0.0.1" };
		uint16_t Port{ 9090u };
		bool IsEnableNagle{ true };
		// TODO(pyoung): keep alive 옵션 으로 빼기.
		KeepAliveOption KeepAliveOption { };
		uint32_t MaxPacketBodyLength{ 2048u };
		uint32_t NetworkIOThreadCount{ 2u };
		uint32_t MaxSessionCount{ 150u };
		uint8_t PacketCode{ 0x00u };
		uint8_t PacketEncodeKey{ 0x00u };
	};
} // namespace nf::network::server::share