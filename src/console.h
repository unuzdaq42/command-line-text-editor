#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#include <cstddef>
#include <thread>
#include <Windows.h>

#include <iostream>

#include <chrono>
#include <sstream>
#include <cstring>
#include <string>
#include <cctype>

#include <vector>

class console
{
public:

	console(const short width, const short height, const short fontW, const short fontH) : 
	m_screenBuffer(width, height), m_fontWidth(fontW), m_fontHeight(fontH) {}

	~console()
	{
		CloseHandle(m_handleOut);

		SetConsoleActiveScreenBuffer(m_oldHandleOut);
		SetConsoleMode(m_handleIn, m_oldInputHandleMode);
	}

	console(const console&) = delete;
	console& operator= (const console&) = delete;

public:

	[[nodiscard]] bool m_construct() noexcept
	{
		m_oldHandleOut = GetStdHandle(STD_OUTPUT_HANDLE);
		m_handleIn = GetStdHandle(STD_INPUT_HANDLE);

		m_handleOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);

		if (
			m_oldHandleOut == INVALID_HANDLE_VALUE ||
			m_handleOut == INVALID_HANDLE_VALUE ||
			m_handleIn == INVALID_HANDLE_VALUE)
			return m_reportLastError();

		if (!GetConsoleMode(m_handleIn, &m_oldInputHandleMode)) return m_reportLastError();

		if (!SetConsoleMode(m_handleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT)) return m_reportLastError();

		SMALL_RECT consoleWindow = { 0, 0, 1, 1 };

		if (!SetConsoleWindowInfo(m_handleOut, TRUE, &consoleWindow)) return m_reportLastError();

		if (!SetConsoleScreenBufferSize(m_handleOut, m_screenBuffer.m_bufferCoord())) return m_reportLastError();

		if (!SetConsoleActiveScreenBuffer(m_handleOut)) return m_reportLastError();

		CONSOLE_FONT_INFOEX cfi = {};

		cfi.cbSize = sizeof(cfi);
		cfi.nFont = 0;
		cfi.dwFontSize = { static_cast<SHORT>(m_fontWidth), static_cast<SHORT>(m_fontHeight) };
		cfi.FontFamily = FF_DONTCARE;
		cfi.FontWeight = FW_NORMAL;

		wcscpy_s(cfi.FaceName, L"Consolas");
		if (!SetCurrentConsoleFontEx(m_handleOut, FALSE, &cfi)) return m_reportLastError();

		CONSOLE_SCREEN_BUFFER_INFO csbi;
		if (!GetConsoleScreenBufferInfo(m_handleOut, &csbi)) return m_reportLastError();

		if (
			m_screenBuffer.m_width > csbi.dwMaximumWindowSize.X ||  
			m_screenBuffer.m_height > csbi.dwMaximumWindowSize.Y)
		{ 
			return m_reportLastError();
		}

		if (!SetConsoleWindowInfo(m_handleOut, TRUE, &m_screenBuffer.m_consoleRect)) return m_reportLastError();

		return true;
	}

	struct screenBuffer
	{
		screenBuffer(const int width, const int height)
		: m_width(width), m_height(height)
		{
			m_buffer = new CHAR_INFO[width * height];
			std::memset(m_buffer, 0, sizeof(CHAR_INFO) * m_bufferSize());

			m_consoleRect = { 0, 0, static_cast<SHORT>(m_width - 1), static_cast<SHORT>(m_height - 1) };
		}

		~screenBuffer()
		{
			delete[] m_buffer;
		}

		screenBuffer(const screenBuffer&) = delete;
		screenBuffer& operator=(const screenBuffer&) = delete;

		using indexType = long;

		static constexpr auto s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
		static constexpr auto s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

		void m_draw(const HANDLE handle) noexcept
		{	
			using namespace std::chrono;

			static auto s_cursorTimer = std::chrono::steady_clock::now();
			static bool s_cursorOn = false;

			if(duration_cast<duration<double>>(steady_clock::now() - s_cursorTimer).count() >= 0.75)
			{
				s_cursorOn = !s_cursorOn;

				s_cursorTimer = steady_clock::now();
			}

			// draw cursor
			if(s_cursorOn)
			{
				const auto oldState = m_buffer[m_currPos];
				m_setPixel(m_currPos, L' ', s_backgroundWhite);

				m_drawBuffer(handle);
			
				m_setPixel(m_currPos, oldState.Char.UnicodeChar, oldState.Attributes);
			}
			else m_drawBuffer(handle);

			
		}

		constexpr void m_clear(const wchar_t value = L' ', const WORD color = s_foregroundWhite) noexcept
		{
			for(indexType i = 0; i < m_width * m_height; ++i)
			{
				m_setPixel(i, value, color);
			}

		}

		constexpr void m_setPixel(const indexType pos, const wchar_t value, const WORD color = s_foregroundWhite) noexcept
		{
			m_buffer[pos].Attributes = color;
			m_buffer[pos].Char.UnicodeChar = value;
		}

		[[nodiscard]] constexpr wchar_t m_getPixel(const indexType pos) const noexcept
		{
			return m_buffer[pos].Char.UnicodeChar;
		}

		[[nodiscard]] constexpr std::size_t m_bufferSize() const noexcept
		{
			return static_cast<std::size_t>(m_width * m_height);
		}
		[[nodiscard]] constexpr COORD m_bufferCoord() const noexcept
		{
			return { static_cast<SHORT>(m_width), static_cast<SHORT>(m_height) };
		}

		// get index and value to set
		constexpr void m_insertChar(const wchar_t value, const WORD color = s_foregroundWhite) noexcept
		{
			for(indexType i = m_currSize - 1; i >= m_currPos; --i)
			{
				m_buffer[i + 1] = m_buffer[i];
			}

			m_setPixel(m_currPos, value, color);

			++m_currSize;
		}

		constexpr void m_handleEvents(const KEY_EVENT_RECORD keyEvent) noexcept
		{
			if (!keyEvent.bKeyDown) return;

			switch (keyEvent.wVirtualKeyCode)
			{
			case VK_BACK: // backspace
				
				if(m_currPos > 0)
				{
					for(indexType i = m_currPos; i < m_currSize; ++i)
					{
						m_buffer[i - 1] = m_buffer[i];
					}
					
					m_setPixel(m_currSize - 1, L' ');

					--m_currSize;
					--m_currPos;
				}

				break;
			case VK_DELETE:
				m_setPixel(m_currPos, L' ');
				break;
			case VK_LEFT:
				if(m_currPos > 0) --m_currPos;			
				break;
			case VK_RIGHT:
				if(m_currPos + 1 < m_currSize) ++m_currPos;
				break;
			default:

				if(keyEvent.uChar.UnicodeChar)
				{
					m_insertChar(keyEvent.uChar.UnicodeChar);

					++m_currPos;
				}


				break;
			}
		}

	public:

		SMALL_RECT m_consoleRect;

		int m_width;
		int m_height;

	
		CHAR_INFO* m_buffer = nullptr;

		indexType m_currPos = 0;
		indexType m_currSize = 1;

		// std::vector<CHAR_INFO> m_buffer;

	private:

		void m_drawBuffer(const HANDLE handle) noexcept
		{
			WriteConsoleOutput(handle, m_buffer, m_bufferCoord(), { 0, 0 }, &m_consoleRect);
		}

	};

	void m_run() noexcept
	{
        using namespace std::chrono;

		m_screenBuffer.m_clear();

		auto frameStartTime = steady_clock::now();
	
		while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01))
		{
			// events

			DWORD eventCount = 0;

			// check errors
			GetNumberOfConsoleInputEvents(m_handleIn, &eventCount);

			if (eventCount > 0)
			{
				INPUT_RECORD inputBuffer[32];

				ReadConsoleInput(m_handleIn, inputBuffer, 32, &eventCount);

				for (std::size_t i = 0; i < eventCount; ++i)
				{
					if (inputBuffer[i].EventType != KEY_EVENT) continue;

					m_screenBuffer.m_handleEvents(inputBuffer[i].Event.KeyEvent);

				}
			}

			// ~events


			m_screenBuffer.m_draw(m_handleOut);

			const auto elapsedTime = duration_cast<duration<double, std::milli>>(steady_clock::now() - frameStartTime);
			frameStartTime = steady_clock::now();

			std::wstringstream ss;

			CONSOLE_SCREEN_BUFFER_INFO csbi;

			GetConsoleScreenBufferInfo(m_handleOut, &csbi);

			ss << L"unuzdaq{ curSize = " << m_screenBuffer.m_currSize << L", currPos = " << m_screenBuffer.m_currPos << L", cursorX = " << csbi.dwCursorPosition.X << L", cursorPosY = " << csbi.dwCursorPosition.Y << L" }";

			if (!SetConsoleTitle(ss.str().c_str())) return;

		}

	}

private:

	bool m_reportLastError() const noexcept
	{
		DWORD errorId = GetLastError();

		LPWSTR errorMessage = nullptr;

		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			nullptr, errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPWSTR>(&errorMessage), 0, nullptr);

		std::wcerr << L"Error = " << errorMessage;

		LocalFree(errorMessage);

		return false;
	}

	screenBuffer m_screenBuffer;

	int m_fontWidth;
	int m_fontHeight;

	HANDLE m_handleOut;
	HANDLE m_handleIn;
	
	HANDLE m_oldHandleOut;
	DWORD m_oldInputHandleMode;
};

