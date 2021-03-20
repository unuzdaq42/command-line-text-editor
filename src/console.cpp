#include "console.h"

#include <cstring>
#include <sstream>
#include <cwchar>
#include <chrono>

#define CONSOLE_ASSERT(x, ...)\
    if (!(x)) { m_reportLastError(#x, __FILE__, __LINE__, __VA_ARGS__); return false; }


Console::~Console() 
{
    CloseHandle(m_handleOut);

    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
    // SetConsoleMode(m_handleIn, m_oldInputHandleMode);
}

[[nodiscard]] bool Console::m_construct(
    const int width, const int height, 
    const short fontW, const short fontH, const bool visibleCursor) noexcept
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
    wcscpy_s(cfi.FaceName, m_consoleFont.c_str());

    CONSOLE_SCREEN_BUFFER_INFO csbi;

    auto rect = m_consoleRect();


    constexpr auto inputHandleModes = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;


    CONSOLE_ASSERT( m_handleOut != INVALID_HANDLE_VALUE && m_handleIn != INVALID_HANDLE_VALUE );


    CONSOLE_ASSERT( GetConsoleMode              ( m_handleIn , &m_oldInputHandleMode    ) );
    CONSOLE_ASSERT( SetConsoleMode              ( m_handleIn , inputHandleModes         ) );
    CONSOLE_ASSERT( SetConsoleWindowInfo        ( m_handleOut, true, &consoleWindow     ) );
    CONSOLE_ASSERT( SetConsoleScreenBufferSize  ( m_handleOut, m_consoleSizeCoord()     ) );
    CONSOLE_ASSERT( SetConsoleActiveScreenBuffer( m_handleOut                           ) );
    CONSOLE_ASSERT( SetCurrentConsoleFontEx     ( m_handleOut, false, &cfi              ) );
    CONSOLE_ASSERT( GetConsoleScreenBufferInfo  ( m_handleOut, &csbi                    ) );
    CONSOLE_ASSERT( SetConsoleWindowInfo        ( m_handleOut, true, &rect              ) );
    

    CONSOLE_ASSERT( m_width <= csbi.dwMaximumWindowSize.X && m_height <= csbi.dwMaximumWindowSize.Y , 
    L"window width or height is above the maximum windows size limit maxSize = {", 
    csbi.dwMaximumWindowSize.X, L", ", csbi.dwMaximumWindowSize.Y, L"}");

    if (!visibleCursor)
    {
        // make cursor invisible
        CONSOLE_CURSOR_INFO cursorInfo;
        cursorInfo.dwSize = 1;
        cursorInfo.bVisible = false;

        CONSOLE_ASSERT(SetConsoleCursorInfo(m_handleOut, &cursorInfo));
    }

    return true;
}

void Console::m_run() noexcept
{
    using namespace std::chrono;

    if (!m_childConstruct()) return;

    auto frameStartTime = steady_clock::now();

    float elapsedTime = 0.0f;

    while (m_runing)
    {
        m_childUpdate(elapsedTime);

        DWORD eventCount = 0;

        GetNumberOfConsoleInputEvents(m_handleIn, &eventCount);

        if (eventCount > 0)
        {
            INPUT_RECORD inputBuffer[32];

            ReadConsoleInputW(m_handleIn, inputBuffer, 32, &eventCount);

            for (std::size_t i = 0; i < eventCount; ++i)
            {
                m_handleEvents(inputBuffer[i]);
            }
        }
        
        CONSOLE_SCREEN_BUFFER_INFO csbi = {};
        GetConsoleScreenBufferInfo(m_handleOut, &csbi);

        m_resizeConsole( { static_cast<short>(csbi.srWindow.Right + 1), static_cast<short>(csbi.srWindow.Bottom + 1) } );

        elapsedTime = duration_cast<duration<float, std::milli>>(steady_clock::now() - frameStartTime).count();
        frameStartTime = steady_clock::now();

        std::wstringstream ss;

        ss << m_consoleTitle << L" (Elapsed Time = " << elapsedTime << L")";

        if (!SetConsoleTitleW(ss.str().c_str())) return;
    }

}

template<typename ... Args>
void Console::m_reportLastError(const char* funcName, const char* fileName, 
    const int lineNumber, const Args& ... args) const noexcept
{
    DWORD errorId = GetLastError();

    LPWSTR errorMessage = nullptr;

    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr, errorId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMessage), 0, nullptr);

    std::wstringstream ss;

    ss << errorMessage << L"\n In = " << fileName << L", At = " 
    << lineNumber << L", while = " << funcName << L"\n";

    if constexpr (sizeof ... (Args) > 0)
    {
        ss << L"Message = ";

        (ss << ... << args) << L"\n";
    }

    MessageBoxW(nullptr, ss.str().c_str(), L"Error", MB_OK | MB_ICONERROR);

    LocalFree(errorMessage);
}



void Console::m_renderConsole() noexcept
{
    auto rect = m_consoleRect();

    WriteConsoleOutputW(m_handleOut, m_screenBuffer.data(), m_consoleSizeCoord(), { 0, 0 }, &rect);
}


void Console::m_handleEvents(const INPUT_RECORD event) noexcept
{

    switch(event.EventType)
    {
    case KEY_EVENT:
        m_childHandleKeyEvents(event.Event.KeyEvent);
    break;
    case MOUSE_EVENT:
        m_childHandleMouseEvents(event.Event.MouseEvent);
    break;
    default:
    break;
    }
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

    m_screenBuffer.resize(static_cast<std::size_t>(width * height));
}