#pragma once

#pragma warning(push, 0)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <array>
#pragma warning(pop)

namespace nf::os::windows::input
{
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	enum class eKey : uint8_t
	{
		S = 0x53,
		P = 0x50,
		Esc = VK_ESCAPE,
	};

	class Keyboard final
	{
	public:
		Keyboard() = default;
		virtual ~Keyboard() = default;
		Keyboard(Keyboard const& other) = delete;
		Keyboard& operator=(Keyboard const& other) = delete;
	public:
		void UpdateKey(eKey const key)
		{
			uint8_t const k = static_cast<uint8_t>(key);
			if (GetAsyncKeyState(k) & 0x8001)
			{
				if (!keyDown[k] && !keyStay[k])
				{
					keyDown[k] = true;
					keyStay[k] = true;
					keyUp[k] = false;
				}
				else
				{
					keyDown[k] = false;
					keyStay[k] = true;
					keyUp[k] = false;
				}
			}
			else
			{
				if (!keyUp[k])
				{
					keyUp[k] = true;
					keyStay[k] = false;
					keyDown[k] = false;
				}
			}
		}
	public:
		inline bool KeyDown(eKey const key) const
		{
			return keyDown[static_cast<uint8_t>(key)];
		}
		inline bool KeyUp(eKey const key) const
		{
			return keyUp[static_cast<uint8_t>(key)];
		}
		inline bool Key(eKey const key) const
		{
			return keyStay[static_cast<uint8_t>(key)];
		}
	private:
		std::array<bool, 0xFF> keyDown{ false, };
		std::array<bool, 0xFF> keyStay{ false, };
		std::array<bool, 0xFF> keyUp{ false, };
	};
} // namespace nf::os::windows::input
