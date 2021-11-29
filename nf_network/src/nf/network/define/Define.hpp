#pragma once

namespace nf::network::define
{
	constexpr int32_t UTF8_CODE_PAGE = 65001;
	constexpr size_t DEFAULT_RECV_BUFFER_SIZE = 3000;
	constexpr size_t DEFAULT_PACKET_MEMORY_POOL_SIZE = 5000;

	constexpr size_t LAN_SESSION_RECVQUEUE_SIZE = 4086;
	constexpr size_t LAN_SESSION_SENDQUEUE_SIZE = 1024;
	constexpr size_t LAN_PACKET_BODY_CAPACITY = 2000;

	constexpr size_t NET_SESSION_RECVQUEUE_SIZE = 8 * 1024; // 8kb;
	constexpr size_t NET_SESSION_SENDQUEUE_SIZE = 1024; // 1kb;
	constexpr size_t NET_PACKET_BODY_CAPACITY = 4 * 1024; // 4kb;

	constexpr size_t MAX_PACKET_COUNT_PER_SEND = 300;
} // namespace nf::network::server::define