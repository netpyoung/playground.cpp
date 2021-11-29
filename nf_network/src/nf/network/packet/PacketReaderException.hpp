#pragma once

#include <exception>

namespace nf::network::packet
{
	struct PacketReaderException : public std::exception
	{
		char const* what() const throw ()
		{
			return "PacketReaderException";
		}
	};
} // namespace nf::network::packet