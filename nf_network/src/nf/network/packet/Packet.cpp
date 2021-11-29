#include "nf/network/packet/Packet.hpp"

namespace nf::network::packet
{
	void PacketEncode(uint8_t const key, uint8_t const randKey, std::byte* inOutBytes, size_t const size)
	{
		uint8_t accK = key;
		uint8_t accRK = randKey;
		uint8_t P = 0;
		uint8_t E = 0;

		for (size_t i = 0; i < size; ++i)
		{
			accK++;
			accRK++;

			P = (P + accRK) ^ static_cast<uint8_t>(*inOutBytes);
			E = (E + accK) ^ P;

			*inOutBytes = static_cast<std::byte>(E);
			inOutBytes++;
		}
	}

	void PacketDecode(uint8_t const key, uint8_t const randKey, std::byte* inOutBytes, size_t const size)
	{
		uint8_t accK = key;
		uint8_t accRK = randKey;

		uint8_t prevE = 0;
		uint8_t prevP = 0;

		for (size_t i = 0; i < size; ++i)
		{
			accK++;
			accRK++;

			uint8_t const currE = static_cast<uint8_t>(*inOutBytes);
			uint8_t const currP = (prevE + accK) ^ currE;
			*inOutBytes = static_cast<std::byte>((prevP + accRK) ^ currP);
			inOutBytes++;

			prevE = currE;
			prevP = currP;
		}
	}

	uint8_t CalculateCheckSum(std::byte* bytes, size_t const size)
	{
		uint64_t checksum = 0;
		for (size_t i = 0; i < size; ++i)
		{
			checksum += static_cast<uint8_t>(*bytes);
			bytes++;
		}
		return static_cast<uint8_t>(checksum % 256);
	}
} // namespace nf::network::packet
