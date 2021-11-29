#pragma once

#include <cstdint>

namespace nf::network::server::share
{
	enum class eServerComponentError : int32_t
	{
		AlreadyRunning,
		NeedToRegisterEventHandler,
		InvalidOption,

		// WSA error
		Fail_WSAStartup,
		Fail_CreateIoCompletionPort_IOHandle,
		Fail_socket_serverSock,
		Fail_bind_serverSock,
		Fail_listen_serverSock,
		Fail_setsockopt,
		Fail_WSAIoctl,
	};
} // namespace nf::network::server::share