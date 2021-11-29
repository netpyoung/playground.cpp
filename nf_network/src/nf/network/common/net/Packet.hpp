#pragma once

#include <cstdint>
#include <nf/network/packet/Packet.hpp>
#include <nf/network/packet/PacketPool.hpp>
#include <nf/network/define/Define.hpp>
#include "Header.hpp"

namespace nf::network::common::net
{
	template <size_t tBodyCapacity>
	using NetPacket = packet::Packet<NetHeader, sizeof(NetHeader), tBodyCapacity>;

	using Packet = NetPacket<define::NET_PACKET_BODY_CAPACITY>;
	using PacketPool = packet::PacketPool<Packet>;
} // namespace nf::network::common::net
