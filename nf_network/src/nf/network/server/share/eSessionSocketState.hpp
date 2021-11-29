#pragma once

namespace nf::network::server::share
{
	// TODO(pyoung): delete this enum
	enum class eSessionSocketState
	{
		Idle,
		Initialized,
		WillClose,
		Closed,
	};
} // namespace nf::network::server::share
