#pragma once

#include <cstdint>
#include <nf/network/packet/Packet.hpp>
#include <nf/network/packet/PacketPool.hpp>
#include <nf/network/define/Define.hpp>
#include "Header.hpp"

namespace nf::network::common::lan
{
	template <size_t tBodyCapacity>
	using LanPacket = packet::Packet<LanHeader, sizeof(LanHeader), tBodyCapacity>;

	using Packet = LanPacket<define::LAN_PACKET_BODY_CAPACITY>;
	using PacketPool = packet::PacketPool<Packet>;
} // namespace nf::network::common::lan
