#pragma once
#include <cstdint>

namespace chat::packet
{
	enum class ePacketType : uint16_t
	{
		CS_CHAT_SERVER = 0,
		CS_CHAT_REQ_LOGIN = 1,
		CS_CHAT_RES_LOGIN = 2,
		CS_CHAT_REQ_SECTOR_MOVE = 3,
		CS_CHAT_RES_SECTOR_MOVE = 4,
		CS_CHAT_REQ_MESSAGE = 5,
		CS_CHAT_RES_MESSAGE = 6,
		CS_CHAT_REQ_HEARTBEAT = 7,
	};

	// ref: WCHAR - utf8 : https://stackoverflow.com/questions/148403/utf8-to-from-wide-char-conversion-in-stl


	// 채팅서버 로그인 요청
	struct CS_CHAT_REQ_LOGIN
	{
		uint16_t Type;
		int64_t AccountNo;
		wchar_t ID[20];// null 포함
		wchar_t Nickname[20];// null 포함
		char SessionKey[64];
	};

	// 채팅서버 로그인 응답
	struct CS_CHAT_RES_LOGIN
	{
		uint16_t Type;
		int8_t Status;	// 0:실패	1:성공
		int64_t AccountNo;
	};

	// 채팅서버 섹터 이동 요청
	struct CS_CHAT_REQ_SECTOR_MOVE
	{
		uint16_t Type;
		int64_t AccountNo;
		uint16_t SectorX;
		uint16_t SectorY;
	};

	// 채팅서버 섹터 이동 결과
	struct CS_CHAT_RES_SECTOR_MOVE
	{
		uint16_t Type;
		int64_t AccountNo;
		uint16_t SectorX;
		uint16_t SectorY;
	};

	// 채팅서버 채팅보내기 요청
	struct CS_CHAT_REQ_MESSAGE
	{
		uint16_t Type;
		int64_t AccountNo;
		uint16_t MessageLen;
		//WCHAR	Message[MessageLen / 2];		// null 미포함
	};

	// 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
	struct CS_CHAT_RES_MESSAGE
	{
		uint16_t Type;

		int64_t AccountNo;
		wchar_t	ID[20];						// null 포함
		wchar_t	Nickname[20];				// null 포함

		uint16_t MessageLen;
		//WCHAR	Message[MessageLen / 2]		// null 미포함
	};

	// 하트비트
	// 클라이언트는 이를 30초마다 보내줌.
	// 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
	struct CS_CHAT_REQ_HEARTBEAT
	{
		uint16_t Type;
	};
} // namespace chat::packet