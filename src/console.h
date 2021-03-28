#ifndef CONSOLE_H
#define CONSOLE_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _WIN32_WINNT 0x0502 /// // // // 

#include <windows.h>
#include <cstddef>
#include <string>
#include <vector>

#define CONSOLE_ASSERT(x, ...)\
    if (!(x)) { Console::s_reportLastError(#x, __FILE__, __LINE__, __VA_ARGS__); return false; }



namespace VirtualKeyCode
{
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	enum KeyCode : WORD
	{
		Num0 = 0x30, Num1 = 0x31,
		Num2 = 0x32, Num3 = 0x33,
		Num4 = 0x34, Num5 = 0x35,
		Num6 = 0x36, Num7 = 0x37,
		Num8 = 0x38, Num9 = 0x39,

		Numpad0 = 0x60, Numpad1 = 0x61,
		Numpad2 = 0x62, Numpad3 = 0x63,
		Numpad4 = 0x64, Numpad5 = 0x65,
		Numpad6 = 0x66, Numpad7 = 0x67,
		Numpad8 = 0x68, Numpad9 = 0x69,
		
		A = 0x41, B = 0x42, C = 0x43, D = 0x44,
		E = 0x45, F = 0x46, G = 0x47, H = 0x48,
		I = 0x49, J = 0x4A, K = 0x4B, L = 0x4C,
		M = 0x4D, N = 0x4E, O = 0x4F, P = 0x50,
		Q = 0x51, R = 0x52, S = 0x53, T = 0x54,
		U = 0x55, V = 0x56, W = 0x57, X = 0x58,
		Y = 0x59, Z = 0x5A,
	};
}

class Console
{
protected:

	static constexpr auto s_defalutConsoleMode = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
	
public:

    Console() = default;
	virtual ~Console();

	Console(const Console&) = delete;
	Console& operator= (const Console&) = delete;


    [[nodiscard]] bool m_construct(
		const int width, const int height, 
		const short fontW, const short fontH, 
		const bool visibleCursor = false, const DWORD consoleMode = s_defalutConsoleMode) noexcept;

	void m_run() noexcept;

protected:

    [[nodiscard]] constexpr int m_screenWidth () const noexcept { return m_width;  }
    [[nodiscard]] constexpr int m_screenHeight() const noexcept { return m_height; }

    std::wstring m_consoleFont  = L"Consolas";
    std::wstring m_consoleTitle = L"console";	

	static constexpr WORD s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	static constexpr WORD s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

	void m_setGrid(const std::size_t index, const wchar_t c, 
		const WORD color = s_foregroundWhite) noexcept
	{
		if (index < m_screenBuffer.size())
		{
			m_screenBuffer[index].Attributes       = color;
			m_screenBuffer[index].Char.UnicodeChar = c;
		}
	}

	void m_setGrid(
		const std::size_t x, const std::size_t y, 
		const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		if (x < m_width && y < m_height)
		{
			const std::size_t index = (y * m_width) + x;

			m_screenBuffer[index].Attributes       = color;
			m_screenBuffer[index].Char.UnicodeChar = c;
		}
	}

	void m_drawPixel(const std::size_t index, 
		const bool r, const bool g, const bool b, const bool a) noexcept
	{
		WORD color = 0;

		if (r) color |= BACKGROUND_RED;
		if (g) color |= BACKGROUND_GREEN;
		if (b) color |= BACKGROUND_BLUE;
		if (a) color |= BACKGROUND_INTENSITY;

		m_setGrid(index, L' ', color);
	}

	void m_drawPixel(const std::size_t x, const std::size_t y, 
		const bool r, const bool g, const bool b, const bool a) noexcept
	{
		m_drawPixel((y * m_screenWidth()) + x, r, g, b, a);
	}


    void m_clearConsole() noexcept
	{
		for (std::size_t i = 0; i < m_screenBuffer.size(); ++i)
		{
			m_setGrid(i, L' ');
		}
	}
	
	void m_renderConsole() noexcept;

    constexpr void m_closeConsole() noexcept { m_runing = false; }

	void m_drawString(
		std::size_t x, std::size_t y, const std::wstring_view& str, 
		const WORD color = s_foregroundWhite, const bool wrap = true) noexcept
	{
		for (const auto element : str)
		{
			if (x < m_width)
			{
				m_setGrid(x, y, element, color);
				++x;
			}
			else
			{
				if (!wrap) break;

				x = 0;
				if (++y > m_height) break;
			}
		}


	}

protected:

    virtual bool m_childCreate()            = 0;
	virtual void m_childDestroy()   	    = 0;
    virtual void m_childUpdate(const float) = 0;

    virtual void m_childHandleKeyEvents  (const KEY_EVENT_RECORD&  ) {}
    virtual void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD&) {}
	virtual void m_childHandleResizeEvent(const COORD, const COORD ) {}

protected:

	[[nodiscard]] constexpr auto m_getConsoleHandleOut() const noexcept { return m_handleOut; }
	[[nodiscard]] constexpr auto m_getConsoleHandleIn () const noexcept { return m_handleIn ; }

	void m_setCursorPos(const short x, const short y) const noexcept
	{
		SetConsoleCursorPosition(m_handleOut, { x, y } );
	}

private:

	std::vector<CHAR_INFO> m_screenBuffer;

	int m_width  = 0;
	int m_height = 0;

	COORD m_fontSize = {};

	HANDLE m_handleOut = nullptr;
	HANDLE m_handleIn  = nullptr;
	
	DWORD m_oldInputHandleMode;

    bool m_runing = true;

public:

	template<typename ... Args>
	static void s_reportLastError(const char* funcName, const char* fileName, 
    	const int lineNumber, const Args& ... args) noexcept;

private:

	[[nodiscard]] constexpr COORD m_consoleSizeCoord() const noexcept
	{
		return { static_cast<short>(m_width), static_cast<short>(m_height) };
	}

	[[nodiscard]] SMALL_RECT m_consoleRect() const noexcept
	{
		return { 0, 0, static_cast<short>(m_width - 1), static_cast<short>(m_height - 1) };
	}

private:

	void m_handleEvents() noexcept;

	void m_resizeConsole(const COORD newSize) noexcept;
	void m_createScreenBuffer(const int width, const int height) noexcept;
};


#endif