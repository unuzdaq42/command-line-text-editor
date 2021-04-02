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
#include <optional>

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

public:

	static constexpr WORD s_ctrlKeyFlag = RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED;
	static constexpr WORD s_altKeyFlag  = LEFT_ALT_PRESSED   | RIGHT_ALT_PRESSED;

	[[nodiscard]] static bool s_isCtrlKeyPressed(const KEY_EVENT_RECORD& event) noexcept
    {
        // windows gives ctrl left event when user presses alt gr key
        // couldnt find anything better than this 
        const bool isAltGrKeyPressed = GetKeyState(VK_RMENU) & 0x8000;

        return (event.dwControlKeyState & s_ctrlKeyFlag) != 0 && !isAltGrKeyPressed;
    }

	[[nodiscard]] static constexpr bool s_isAltKeyPressed(const KEY_EVENT_RECORD event) noexcept
	{
		return (event.dwControlKeyState & s_altKeyFlag) != 0;
	}

    [[nodiscard]] static constexpr bool s_isShiftKeyPressed(const KEY_EVENT_RECORD event) noexcept
    {
        return (event.dwControlKeyState & SHIFT_PRESSED) == SHIFT_PRESSED;
    }

    [[nodiscard]] static constexpr bool s_isShiftKeyEvent(const KEY_EVENT_RECORD event) noexcept
    {
        return event.wVirtualKeyCode == VK_SHIFT;
    }

	static bool s_setUserClipboard(const std::wstring_view str) noexcept;
    [[nodiscard]] static std::optional<std::wstring> s_getUserClipboard() noexcept;

public:

    [[nodiscard]] constexpr int m_screenWidth () const noexcept { return m_width;  }
    [[nodiscard]] constexpr int m_screenHeight() const noexcept { return m_height; }

	[[nodiscard]] constexpr std::size_t m_screenBufferSize() const noexcept 
	{ 
		return static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height); 
	}

    std::wstring m_consoleFont  = L"Consolas";
    std::wstring m_consoleTitle = L"console";	

	static constexpr WORD s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	static constexpr WORD s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

	[[nodiscard]] constexpr std::size_t m_getIndex(const std::size_t x, const std::size_t y) const noexcept
	{
		return (y * m_width) + x;
	}

	constexpr void m_setGrid(const std::size_t index, const wchar_t c, 
		const WORD color = s_foregroundWhite) noexcept
	{
		if (index < m_screenBufferSize())
		{
			m_screenBuffer[index].Attributes       = color;
			m_screenBuffer[index].Char.UnicodeChar = c;
		}
	}

	constexpr void m_setGrid(
		const std::size_t x, const std::size_t y, 
		const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		if (x < m_width && y < m_height)
		{
			const std::size_t index = m_getIndex(x, y);

			m_screenBuffer[index].Attributes       = color;
			m_screenBuffer[index].Char.UnicodeChar = c;
		}
	}

	[[nodiscard]] constexpr WORD m_getColorAt(const std::size_t index) const noexcept
	{
		if (index < m_screenBufferSize())
		{
			return m_screenBuffer[index].Attributes;
		}
		
		return 0;
	}

	constexpr void m_setColorAt(const std::size_t index, const WORD color) noexcept
	{
		if (index < m_screenBufferSize())
		{
			m_screenBuffer[index].Attributes = color;
		}
	}

	constexpr void m_setRect(const std::size_t startX, const std::size_t startY,
		const std::size_t width, const std::size_t height,
		const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		for (std::size_t i = 0; i < height; ++i)
		{
			for (std::size_t t = 0; t < width; ++t)
			{
				m_setGrid(startX + t, startY + i, c, color);
			}
		}
	}

	[[nodiscard]] static constexpr WORD s_getColorValue(
		const bool r, const bool g, const bool b, const bool a) noexcept
	{
		WORD color = 0;

		if (r) color |= BACKGROUND_RED;
		if (g) color |= BACKGROUND_GREEN;
		if (b) color |= BACKGROUND_BLUE;
		if (a) color |= BACKGROUND_INTENSITY;

		return color;
	}

	constexpr void m_drawPixel(const std::size_t index, 
		const bool r, const bool g, const bool b, const bool a) noexcept
	{
		m_setGrid(index, L' ', s_getColorValue(r, g, b, a));
	}

	constexpr void m_drawPixel(const std::size_t x, const std::size_t y, 
		const bool r, const bool g, const bool b, const bool a) noexcept
	{
		m_drawPixel((y * m_screenWidth()) + x, r, g, b, a);
	}

	constexpr void m_drawRect(const std::size_t startX, const std::size_t startY,
		const std::size_t width, const std::size_t height, const WORD color = s_foregroundWhite) noexcept
	{
		m_setRect(startX, startY, width, height, L' ', color);
	}

    constexpr void m_clearConsole() noexcept
	{
		for (std::size_t i = 0; i < m_screenBufferSize(); ++i)
		{
			m_setGrid(i, L' ');
		}
	}
	
	void m_renderConsole() noexcept;

    constexpr void m_closeConsole() noexcept { m_runing = false; }

	constexpr void m_drawString(
		std::size_t x, std::size_t y, const std::wstring_view str, 
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

public:

	void m_setCursorPos(const COORD pos) const noexcept
	{
		SetConsoleCursorPosition(m_handleOut, pos);
	}

	void m_setFontSize(const short w, const short h) const noexcept
	{
		CONSOLE_FONT_INFOEX cfi;

		cfi.cbSize = sizeof(cfi);

		GetCurrentConsoleFontEx(m_handleOut, FALSE, &cfi);

		cfi.dwFontSize = { w, h };

		SetCurrentConsoleFontEx(m_handleOut, FALSE, &cfi);
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