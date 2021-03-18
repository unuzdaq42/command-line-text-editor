#ifndef CONSOLE_H
#define CONSOLE_H

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <cstddef>
#include <string>

class Console
{
public:

    Console() = default;
	virtual ~Console();

	Console(const Console&) = delete;
	Console& operator= (const Console&) = delete;


    [[nodiscard]] bool m_construct(
		const int width, const int height, 
		const short fontW, const short fontH, const bool visibleCursor = false) noexcept;

	void m_run() noexcept;

protected:

    std::wstring m_consoleFont  = L"Consolas";
    std::wstring m_consoleTitle = L"console";

	static constexpr auto s_foregroundWhite = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; 
	static constexpr auto s_backgroundWhite = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE;

	constexpr void m_setGrid(const std::size_t index, const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		if (index < m_bufferSize())
		{
			m_screenBuffer[index].Attributes = color;
			m_screenBuffer[index].Char.UnicodeChar = c;
		}
	}

	constexpr void m_setGrid(const std::size_t x, const std::size_t y, const wchar_t c, const WORD color = s_foregroundWhite) noexcept
	{
		m_setGrid((y * m_width) + x, c, color);
	}

	constexpr void m_drawPixel(const std::size_t index, const bool r, const bool g, const bool b, const bool a) noexcept
	{
		WORD color = 0;

		if (r) color |= BACKGROUND_RED;
		if (g) color |= BACKGROUND_GREEN;
		if (b) color |= BACKGROUND_BLUE;
		if (a) color |= BACKGROUND_INTENSITY;

		m_setGrid(index, L' ', color);
	}

    [[nodiscard]] constexpr int m_screenWidth () const noexcept { return m_width;  }
    [[nodiscard]] constexpr int m_screenHeight() const noexcept { return m_height; }

    constexpr void m_clearConsole() noexcept
	{
		for (std::size_t i = 0; i < m_bufferSize(); ++i)
		{
			m_setGrid(i, L' ');
		}
	}
	
	void m_renderConsole() noexcept;

    constexpr void m_closeConsole() noexcept { m_runing = false; }

protected:

    virtual bool m_childConstruct()            = 0;
    virtual void m_childUpdate   (const float) = 0;

    virtual void m_childHandleKeyEvents  (const KEY_EVENT_RECORD  ) {}
    virtual void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD) {}
	virtual void m_childHandleResizeEvent(const COORD) 				{}

protected:

	[[nodiscard]] constexpr auto m_getConsoleHandleOut() const noexcept { return m_handleOut; }
	[[nodiscard]] constexpr auto m_getConsoleHandleIn () const noexcept { return m_handleIn ; }

private:

	CHAR_INFO* m_screenBuffer = nullptr;
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

	[[nodiscard]] constexpr std::size_t m_bufferSize() const noexcept
	{
		return static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
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

    void m_handleEvents(const INPUT_RECORD event) noexcept;

	void m_resizeConsole(const COORD newSize) noexcept;
};


#endif