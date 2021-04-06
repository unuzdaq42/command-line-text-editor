#include "../include/console.h"

#include <cstring>
#include <sstream>
#include <cwchar>
#include <chrono>

#define CONSOLE_ASSERT(x, ...)\
    if (!(x)) { Console::s_reportLastError(#x, __FILE__, __LINE__, __VA_ARGS__); return false; }

Console::~Console() 
{
    CloseHandle(m_handleOut);

    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    SetConsoleMode(m_handleIn, m_oldInputHandleMode);
}

[[nodiscard]] bool Console::m_construct(
    const int width, const int height, 
    const short fontW, const short fontH, 
    const bool visibleCursor, const DWORD consoleMode) noexcept
{
    m_fontSize = { fontW, fontH };

    m_createScreenBuffer(width, height);

    m_handleIn = GetStdHandle(STD_INPUT_HANDLE);

    m_handleOut = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CONSOLE_TEXTMODE_BUFFER, nullptr);

    SMALL_RECT consoleWindow = { 0, 0, 1, 1 };

    CONSOLE_FONT_INFOEX cfi = {};

    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize = m_fontSize;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    auto rect = m_consoleRect();

    CONSOLE_ASSERT( m_handleOut != INVALID_HANDLE_VALUE && m_handleIn != INVALID_HANDLE_VALUE );


    CONSOLE_ASSERT( GetConsoleMode              ( m_handleIn , &m_oldInputHandleMode    ) );
    CONSOLE_ASSERT( SetConsoleMode              ( m_handleIn , consoleMode              ) );
    CONSOLE_ASSERT( SetConsoleWindowInfo        ( m_handleOut, true, &consoleWindow     ) );
    CONSOLE_ASSERT( SetConsoleScreenBufferSize  ( m_handleOut, m_consoleSizeCoord()     ) );
    CONSOLE_ASSERT( SetConsoleActiveScreenBuffer( m_handleOut                           ) );
    CONSOLE_ASSERT( SetCurrentConsoleFontEx     ( m_handleOut, false, &cfi              ) );
    CONSOLE_ASSERT( GetConsoleScreenBufferInfo  ( m_handleOut, &csbi                    ) );
    CONSOLE_ASSERT( SetConsoleWindowInfo        ( m_handleOut, true, &rect              ) );
    

    CONSOLE_ASSERT( m_width <= csbi.dwMaximumWindowSize.X && m_height <= csbi.dwMaximumWindowSize.Y , 
    L"window width or height is above the maximum windows size limit maxSize = {", 
    csbi.dwMaximumWindowSize.X, L", ", csbi.dwMaximumWindowSize.Y, L"}");

    CONSOLE_ASSERT(m_setCursorInfo(visibleCursor));
    
    return true;
}

void Console::m_run() noexcept
{
    while (m_runing)
    {
        m_handleEvents();
    }    
}

constexpr void Console::MouseButton::m_handleEvent(const MOUSE_EVENT_RECORD& event) noexcept
{
    if (event.dwButtonState == FROM_LEFT_1ST_BUTTON_PRESSED)
    {
        if (!m_buttonPressed)
        {
            m_timePoint = std::chrono::steady_clock::now();
            m_buttonPressed = true;
        }
    }
    else if (event.dwEventFlags == 0)
    {
        m_buttonPressed = false;
    }

}


template<typename ... Args>
void Console::s_reportLastError(const char* funcName, const char* fileName, 
    const int lineNumber, const Args& ... args) noexcept
{
    
    std::wstringstream ss;

    DWORD errorId = GetLastError();

    if (errorId)
    {
        LPWSTR errorMessage = nullptr;

        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr, errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&errorMessage), 0, nullptr);

        ss << L"Windows error message = " << errorMessage << L"\n";

        LocalFree(errorMessage);
    }

    ss << "In = " << fileName << L", At = " 
    << lineNumber << L", while = " << funcName << L"\n";

    if constexpr (sizeof ... (Args) > 0)
    {
        ss << L"Message = ";

        (ss << ... << args) << L"\n";
    }

    MessageBoxW(nullptr, ss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);
}



void Console::m_renderConsole() noexcept
{
    auto rect = m_consoleRect();

    WriteConsoleOutputW(m_handleOut, m_screenBuffer.data(), m_consoleSizeCoord(), { 0, 0 }, &rect);
}

void Console::m_handleEvents() noexcept
{
    DWORD eventCount = 0;

    GetNumberOfConsoleInputEvents(m_handleIn, &eventCount);

    if (eventCount > 0)
    {
        INPUT_RECORD inputBuffer[32];

        ReadConsoleInputW(m_handleIn, inputBuffer, 32, &eventCount);

        for (std::size_t i = 0; i < eventCount; ++i)
        {
            const auto& event = inputBuffer[i];

            switch(event.EventType)
            {
            case KEY_EVENT:
                
                m_childHandleKeyEvents(event.Event.KeyEvent);
                break;
            case MOUSE_EVENT:
            {
                const auto& mouseEvent = event.Event.MouseEvent;

                m_leftMouseButton.m_handleEvent (mouseEvent);
                m_rightMouseButton.m_handleEvent(mouseEvent);

                m_childHandleMouseEvents(mouseEvent);
                
                break;
            }
            default:
                break;
            }
        }
    }

    CONSOLE_SCREEN_BUFFER_INFO csbi = {};
    GetConsoleScreenBufferInfo(m_handleOut, &csbi);

    m_resizeConsole( { static_cast<short>(csbi.srWindow.Right + 1), static_cast<short>(csbi.srWindow.Bottom + 1) } );
}

void Console::m_resizeConsole(const COORD newSize) noexcept
{
    if (newSize.X != m_width || newSize.Y != m_height)
    {
		const auto oldCoord = m_consoleSizeCoord();
        
        m_createScreenBuffer(newSize.X, newSize.Y);
		SetConsoleScreenBufferSize(m_handleOut, newSize);
        m_childHandleResizeEvent(oldCoord, newSize);
    }
}


void Console::m_createScreenBuffer(const int width, const int height) noexcept
{
    m_width  = width;   
    m_height = height;

    m_screenBuffer.resize(m_screenBufferSize());
}

bool Console::s_setUserClipboard(const std::wstring_view str) noexcept
{
	if (!OpenClipboard(nullptr)) return false;

	const auto size = (str.size() + 1) * sizeof(wchar_t);

	const auto stringHandle = GlobalAlloc(GMEM_MOVEABLE, size);

	if (!stringHandle) return false;

	const auto lockedStr = GlobalLock(stringHandle);

	if (!lockedStr) return false;

	std::memcpy(lockedStr, str.data(), size);

	GlobalUnlock(stringHandle);

	if (!SetClipboardData(CF_UNICODETEXT, stringHandle)) return false;

	CloseClipboard();

	return true;
}

[[nodiscard]] std::optional<std::wstring> Console::s_getUserClipboard() noexcept
{	
	if (!OpenClipboard(nullptr)) return {};

	const auto clipboardHandle = GetClipboardData(CF_UNICODETEXT);
	
	if (!clipboardHandle) return {};
	
	const auto data = static_cast<wchar_t*>(GlobalLock(clipboardHandle));
	
	if (!data) return {};

	std::optional<std::wstring> result = { data };

	GlobalUnlock(clipboardHandle);

	CloseClipboard();

	return result;
}