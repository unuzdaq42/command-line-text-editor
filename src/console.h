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
#include <algorithm>

#include <vector>

// remove noexcept from string stuff or whatever

class console
{
public:

	console(const int width, const int height, const short fontW, const short fontH) : 
	m_width(width), m_height(height), m_fontWidth(fontW), m_fontHeight(fontH) 
	{
		m_screenBuffer = new CHAR_INFO[m_bufferSize()];
		std::memset(m_screenBuffer, 0, sizeof(CHAR_INFO) * m_bufferSize());
	}

	~console()
	{
		delete[] m_screenBuffer;

		CloseHandle(m_handleOut);

		SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
		SetConsoleMode(m_handleIn, m_oldInputHandleMode);
	}

	console(const console&) = delete;
	console& operator= (const console&) = delete;

private:

	static constexpr auto s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	static constexpr auto s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

	constexpr void m_setGrid(const std::size_t index, const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		m_screenBuffer[index].Attributes = color;
		m_screenBuffer[index].Char.UnicodeChar = c;
	}

	constexpr void m_setGrid(const std::size_t x, const std::size_t y, const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		m_setGrid((y * m_width) + x, c, color);
	}

	constexpr void m_emptyGrid(const std::size_t index) noexcept
	{
		m_setGrid(index, L' ');
	} 

public:

	[[nodiscard]] bool m_construct() noexcept
	{
		m_handleIn = GetStdHandle(STD_INPUT_HANDLE);

		m_handleOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);

		if (m_handleOut == INVALID_HANDLE_VALUE ||
			m_handleIn == INVALID_HANDLE_VALUE)
			return m_reportLastError();

		if (!GetConsoleMode(m_handleIn, &m_oldInputHandleMode)) return m_reportLastError();

		if (!SetConsoleMode(m_handleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT)) return m_reportLastError();

		SMALL_RECT consoleWindow = { 0, 0, 1, 1 };

		if (!SetConsoleWindowInfo(m_handleOut, TRUE, &consoleWindow)) return m_reportLastError();

		if (!SetConsoleScreenBufferSize(m_handleOut, m_bufferCoord())) return m_reportLastError();

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

		if (m_width > csbi.dwMaximumWindowSize.X || m_height > csbi.dwMaximumWindowSize.Y)
		{ 
			return m_reportLastError();
		}

		if (!SetConsoleWindowInfo(m_handleOut, TRUE, m_ptrConsoleRect())) return m_reportLastError();

		// CONSOLE_CURSOR_INFO cursorInfo;
		// cursorInfo.dwSize = 1;
		// cursorInfo.bVisible = FALSE;

		// if(!SetConsoleCursorInfo(m_handleOut, &cursorInfo)) return m_reportLastError();

		return true;
	}

	void m_run() noexcept
	{
        using namespace std::chrono;

		auto frameStartTime = steady_clock::now();
	
		while (!(GetAsyncKeyState(VK_ESCAPE) & 0x01))
		{
			// events

			DWORD eventCount = 0;

			// check errors
			GetNumberOfConsoleInputEvents(m_handleIn, &eventCount);

			bool updateConsole = false;

			if (eventCount > 0)
			{
				INPUT_RECORD inputBuffer[32];

				ReadConsoleInput(m_handleIn, inputBuffer, 32, &eventCount);

				for (std::size_t i = 0; i < eventCount; ++i)
				{
					if (inputBuffer[i].EventType != KEY_EVENT) continue;

					updateConsole |= m_handleEvents(inputBuffer[i].Event.KeyEvent);

					// m_screenBuffer.m_handleEvents(inputBuffer[i].Event.KeyEvent);

				}
			}

			// ~events

			// update if anything is changed
			if(updateConsole) { m_updateConsole(); }
			
			const auto elapsedTime = duration_cast<duration<double, std::milli>>(steady_clock::now() - frameStartTime);
			frameStartTime = steady_clock::now();

			std::wstringstream ss;

			CONSOLE_SCREEN_BUFFER_INFO csbi;

			GetConsoleScreenBufferInfo(m_handleOut, &csbi);

			ss << m_consoleInputs.size() << L", " << m_rowCount << L", " << m_startRow << L", " << m_inputIndex << L", " << csbi.dwCursorPosition.X << L", " << csbi.dwCursorPosition.Y;

			if (!SetConsoleTitle(ss.str().c_str())) return;

			std::this_thread::sleep_for(milliseconds(16));
		}

	}

private:

	CHAR_INFO* m_screenBuffer;
	int m_width;
	int m_height;

	short m_fontWidth;
	short m_fontHeight;

	HANDLE m_handleOut;
	HANDLE m_handleIn;
	
	DWORD m_oldInputHandleMode;

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

	[[nodiscard]] constexpr std::size_t m_bufferSize() const noexcept
	{
		return static_cast<std::size_t>(m_width * m_height);
	}

	[[nodiscard]] constexpr COORD m_bufferCoord() const noexcept
	{
		return { static_cast<short>(m_width), static_cast<short>(m_height) };
	}

	[[nodiscard]] SMALL_RECT* m_ptrConsoleRect() const noexcept
	{
		static SMALL_RECT rect = { 0, 0, static_cast<short>(m_width - 1), static_cast<short>(m_height - 1) };

		return &rect;
	}

private:

	std::wstring m_consoleInputs;

	std::size_t m_inputIndex = 0;

	std::size_t m_startRow = 0;
	std::size_t m_rowCount = 1;

	COORD m_cursorPos = {};

private:

	constexpr void m_clearConsole() noexcept
	{
		for(std::size_t i = 0; i < m_bufferSize(); ++i)
		{
			m_emptyGrid(i);
		}
	}

	[[nodiscard]] constexpr std::size_t m_getStartIndex() const noexcept
	{
		std::size_t result = 0;

		if(m_startRow > 0)
		{

			std::size_t rowCount = 0;
			
			for(const auto& elem :  m_consoleInputs)
			{
				++result;

				if(elem == L'\n')
				{
					if(m_startRow == ++rowCount) break;    
				}
			}

		}

		return result;
	} 

	void m_updateConsole() noexcept
	{
		m_clearConsole();

		std::size_t i = 0;
		std::size_t t = 0;

		const std::size_t startIndex = m_getStartIndex();

		for(auto it = m_consoleInputs.cbegin() + startIndex; it != m_consoleInputs.cend(); ++it)
		{
			if(*it == '\n')
			{
				++i;
				t = 0;
			
				if(i >= m_height) break;

				continue;
			}

			if(t >= m_width) continue;

			m_setGrid(t, i, *it);

			++t;
		}

		SetConsoleCursorPosition(m_handleOut, m_cursorPos);

		// check returned error value
		WriteConsoleOutputW(m_handleOut, m_screenBuffer, m_bufferCoord(), { 0, 0 }, m_ptrConsoleRect());
	}

	[[nodiscard]] std::size_t m_getRowNumAtIndex(const std::size_t index) const noexcept
	{
		if(index >= m_consoleInputs.size()) return 0;

		std::size_t rowCount = 0;

		for(std::size_t i = 0; i <= index; ++i)
		{
			if(m_consoleInputs[i] == L'\n') ++rowCount;
		}

		return rowCount;
	}

	[[nodiscard]] std::size_t m_getColSizeAtRow(const std::size_t row) const noexcept
	{
		std::size_t result = 0;
		std::size_t rowCount = 0;

		for(const auto& elem : m_consoleInputs)
		{
			if(elem == L'\n')
			{
				if(++rowCount == row) break;
				result = 0;
			}
			else ++result;

		}

		return result;
	}

	bool m_handleEvents(const KEY_EVENT_RECORD event) noexcept
	{
		if(!event.bKeyDown) return false;

		switch(event.wVirtualKeyCode)
		{
		case VK_BACK:
			if(m_inputIndex > 0)
			{
				const auto it = m_consoleInputs.begin() + m_inputIndex - 1;

				--m_inputIndex;
				

				if(*it == L'\n')
				{
					if(--m_rowCount < m_height && m_startRow > 0)
					{	
						--m_startRow;
					}
					else --m_cursorPos.Y;

					m_cursorPos.X = static_cast<short>(m_getColSizeAtRow(m_rowCount));

				} else --m_cursorPos.X;
								
				m_consoleInputs.erase(it);

				return true;
			}
			break;
		case VK_DELETE:
			break;
		case VK_LEFT:
			if(m_inputIndex > 0)
			{
				if(m_consoleInputs[--m_inputIndex] == L'\n')
				{
					m_cursorPos.X = 0;
					--m_cursorPos.Y;
				}
				else --m_cursorPos.X;
			}
			return true;
			// break; // NO NEED TO DRAW AGAIN
		case VK_RIGHT:
			if(m_inputIndex < m_consoleInputs.size())
			{
				++m_inputIndex;
				++m_cursorPos.X;
			}
			return true;
			// break;
		case VK_RETURN:

			m_consoleInputs.insert(m_consoleInputs.begin() + m_inputIndex, L'\n');

			++m_inputIndex;			
			
			m_cursorPos.X = 0;

			if(++m_rowCount > m_height)
			{
				++m_startRow;
			}
			else ++m_cursorPos.Y;

			return true;
		default:
			
			if(event.uChar.UnicodeChar) 
			{ 
				m_consoleInputs.insert(m_consoleInputs.begin() + m_inputIndex, event.uChar.UnicodeChar);

				++m_inputIndex;
				++m_cursorPos.X;

				return true; 
			}
			
			break;
		}

		return false;
	}
};

