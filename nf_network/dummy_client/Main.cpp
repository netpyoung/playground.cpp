#pragma warning(push, 0)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <WS2tcpip.h>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#pragma warning(pop)

#include "nf/network/common/lan/Packet.hpp"

using namespace nf;

std::optional<int32_t> DomainToIP(std::string const domain, IN_ADDR* const pAddr)
{
	ADDRINFO* pAddrInfo;
	int32_t result = GetAddrInfo(domain.c_str(), "0", NULL, &pAddrInfo);
	if (result != 0)
	{
		return result;
	}

	SOCKADDR_IN* pSockAddr = reinterpret_cast<SOCKADDR_IN*>(pAddrInfo->ai_addr);
	*pAddr = pSockAddr->sin_addr;
	FreeAddrInfo(pAddrInfo);
	return {};
}

int main()
{
	// 윈속 초기화.
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "[ERROR] WSAStartup : " << WSAGetLastError() << std::endl;
		return 1;
	}

	// 소켓 생성.
	SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSock == INVALID_SOCKET)
	{
		std::cout << "[ERROR] socket : " << WSAGetLastError() << std::endl;
		return 1;
	}

	SOCKADDR_IN sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	sockAddr.sin_family = AF_INET;
	InetPton(AF_INET, "127.0.0.1", &sockAddr.sin_addr);
	sockAddr.sin_port = htons(9090);

	// 서버 접속.
	int err = connect(clientSock, reinterpret_cast<sockaddr*>(&sockAddr), sizeof(sockAddr));
	if (err == SOCKET_ERROR)
	{
		std::cout << "[ERROR] connect() : " << WSAGetLastError() << std::endl;
		return 1;
	}

	int32_t count = 0;

	int32_t processByteCount = 0;
	int32_t processPacketCount = 0;

	network::common::lan::Packet packet;
	network::common::lan::Header header;
	header.Length = sizeof(int32_t);


	char buffer[1000 + 1] = { 0, };
	size_t bufferSize = 1000;

	DWORD processTick = GetTickCount();
	while (true)
	{
		packet.ClearBuffer();
		packet.SetHeader(header);

		packet << count;

		std::cout << ">> " << count << std::endl;
		count++;
		//sprintf_s(buffer, bufferSize, "12345");

		int32_t sendResult = send(clientSock, reinterpret_cast<char const*>(packet.GetHeaderPtr()), packet.GetPacketSize(), 0);
		if (sendResult == SOCKET_ERROR)
		{
			std::cout << "[ERROR] send() : " << WSAGetLastError() << std::endl;
			break;
		}
		packet.ClearBuffer();

		int32_t recvResult = recv(clientSock, reinterpret_cast<char*>(packet.GetHeaderPtr()), sizeof(header) + header.Length, 0);
		if (recvResult == SOCKET_ERROR)
		{
			std::cout << "[ERROR] recv() : " << WSAGetLastError() << std::endl;
			break;
		}
		if (recvResult == 0) // 정상종료.
		{
			std::cout << "[Finish] recv() : " << std::endl;
			break;
		}

		packet.MoveBodyWritePos(header.Length);
		int32_t val{ -1 };
		packet >> &val;
		assert(val != -1);

		std::cout << "<< " << val << std::endl;

		Sleep(1000);
		processPacketCount += 1;

		if (GetTickCount() > processTick + 1000)
		{
			std::cout << "==================" << std::endl;
			processByteCount = 0;
			processPacketCount = 0;
			processTick = GetTickCount();
		}
	}
	closesocket(clientSock);
	WSACleanup();
	return 0;
}