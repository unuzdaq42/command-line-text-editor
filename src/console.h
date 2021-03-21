#ifndef CONSOLE_H
#define CONSOLE_H

#ifndef UNICODE
#define UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#define _WIN32_WINNT 0x0502

#include <windows.h>
#include <cstddef>
#include <string>
#include <vector>

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

	static constexpr auto s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	static constexpr auto s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

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

    virtual bool m_childConstruct()            = 0;
    virtual void m_childUpdate   (const float) = 0;

    virtual void m_childHandleKeyEvents  (const KEY_EVENT_RECORD  ) {}
    virtual void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD) {}
	virtual void m_childHandleResizeEvent(const COORD, const COORD) {}

protected:

	[[nodiscard]] constexpr auto m_getConsoleHandleOut() const noexcept { return m_handleOut; }
	[[nodiscard]] constexpr auto m_getConsoleHandleIn () const noexcept { return m_handleIn ; }

private:

	std::vector<CHAR_INFO> m_screenBuffer;

	int m_width  = 0;
	int m_height = 0;

	COORD m_fontSize = {};

	HANDLE m_handleOut = nullptr;
	HANDLE m_handleIn  = nullptr;
	
	DWORD m_oldInputHandleMode;

    bool m_runing = true;

private:

	template<typename ... Args>
	void m_reportLastError(const char* funcName, const char* fileName, 
    	const int lineNumber, const Args& ... args) const noexcept;

	[[nodiscard]] constexpr COORD m_consoleSizeCoord() const noexcept
	{
		return { static_cast<short>(m_width), static_cast<short>(m_height) };
	}

	[[nodiscard]] SMALL_RECT m_consoleRect() const noexcept
	{
		return { 0, 0, static_cast<short>(m_width - 1), static_cast<short>(m_height - 1) };
	}

private:

    void m_handleEvents(const INPUT_RECORD event) noexcept;

	void m_resizeConsole(const COORD newSize) noexcept;
	void m_createScreenBuffer(const int width, const int height) noexcept;
};


#endif