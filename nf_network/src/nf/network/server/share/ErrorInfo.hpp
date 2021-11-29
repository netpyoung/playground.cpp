#pragma once

#include <cstdint>
#include <string>

namespace nf::network::server::share
{
	struct ErrorInfo
	{
		int32_t Error;
		std::string Message;
	};
} // namespace nf::network::server::share