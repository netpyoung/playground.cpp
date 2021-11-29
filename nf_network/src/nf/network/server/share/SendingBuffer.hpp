#pragma once

#include <WinSock2.h>
#include <cstdint>
#include <array>
#include <nf/network/define/Define.hpp>

namespace nf::network::server::share
{
	struct SendingBuffer
	{
		std::array<WSABUF, define::MAX_PACKET_COUNT_PER_SEND > Buffers;
		uint32_t BufferSize;
		::OVERLAPPED* Overlapped;
	};
} // namespace nf::network::server::share
