#pragma once

#include <gtest/gtest.h>
#include "nf/network/packet/Packet.hpp"

namespace nf::network::packet::test
{
	TEST(EncoderTest, ZeroAndNegativeNos)
	{
		char const* const plainText = "aaaaaaaaaabbbbbbbbbbcccccccccc1234567890abcdefghijklmn\0";

		char data[55];

		memcpy(data, plainText, 55);

		uint8_t key = 0xa9;
		uint8_t randKey = 0x31;
		PacketEncode(key, randKey, reinterpret_cast<std::byte*>(data), 55);
		//f9 43 95 8c 5f f3 f7 44 b1 87 46 23 ad b5 1e 01 c1 a3 1e 3f b4 80 18 1b b2 ac 36 0b 8c 9c 4a 5e 84 84 7a 0e 74 84 72 0c 16 a8 82 68 c6 ac 72 74 86 20 32 50 86 04 2d |
		
		PacketDecode(key, randKey, reinterpret_cast<std::byte*>(data), 55);
		// 61 61 61 61 61 61 61 61 61 61 62 62 62 62 62 62 62 62 62 62 63 63 63 63 63 63 63 63 63 63 31 32 33 34 35 36 37 38 39 30 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 00

		ASSERT_STREQ(plainText, data);
	}
}