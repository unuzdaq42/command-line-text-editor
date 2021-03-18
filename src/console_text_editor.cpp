#include "console_text_editor.h"



[[nodiscard]] bool ConsoleTextEditor::m_constructEditor(const int argc, const char* argv[]) noexcept
{
    int width  = 100;
    int height = 50;
    
    short fontW = 8;
    short fontH = 16;

    if(argc > 2)
    {
        width  = std::atoi(argv[1]);
        height = std::atoi(argv[2]);
    
        if(argc > 4)
        {
            fontW = static_cast<short>(std::atoi(argv[3]));
            fontH = static_cast<short>(std::atoi(argv[4]));
        }
    }

    return m_construct(width, height, fontW, fontH, true);
}

bool ConsoleTextEditor::m_childConstruct()
{
    m_inputBuffer.push_back(L' ');

    return true;
}

void ConsoleTextEditor::m_childUpdate(const float) {}

void ConsoleTextEditor::m_childHandleKeyEvents(const KEY_EVENT_RECORD event) 
{
    if (!event.bKeyDown) return;

    switch (event.wVirtualKeyCode)
    {
    case VK_ESCAPE:
        m_closeConsole();
        return;
    case VK_BACK:

        if (m_currentIndex > 0)
        {
            m_deleteCharAt(m_currentIndex - 1);
            --m_currentIndex;
        }

        break;
    case VK_DELETE:
    
        if (m_inputBuffer.size() > m_currentIndex + 1)
        {
            m_deleteCharAt(m_currentIndex);
        }
        break;
    case VK_LEFT:

        if (m_currentIndex > 0) --m_currentIndex;
        break;
    case VK_RIGHT:

        if (m_currentIndex + 1 < m_inputBuffer.size()) ++m_currentIndex;
        break;
    case VK_RETURN:

        m_insertChar(L'\n');

        if (++m_rowCount > m_screenHeight()) ++m_startRow;

        break;
    default:

        if (event.uChar.UnicodeChar)
        {
            m_insertChar(event.uChar.UnicodeChar);
        }
        break;
    }

    m_updateConsole();
}

void ConsoleTextEditor::m_childHandleMouseEvents(const MOUSE_EVENT_RECORD) {}
void ConsoleTextEditor::m_childHandleResizeEvent(const COORD) 			   {}

void ConsoleTextEditor::m_updateScreenBuffer(const std::size_t consoleStartIndex) noexcept
{
    std::size_t i = 0;
    std::size_t t = 0;

    std::size_t currColumnCount = 0;

    const std::size_t columnStartVal = m_getConsoleColumnStartIndex(consoleStartIndex);		

    for (auto it = m_inputBuffer.cbegin() + consoleStartIndex; it != m_inputBuffer.cend(); ++it)
    {
        switch (*it)
        {
        case L'\n':

            if (++i >= m_screenHeight()) return;
            
            t = 0;
            currColumnCount = 0;
            
            break;
        case L'\t':
        {
            const auto oldCount = currColumnCount;

            if ((currColumnCount += s_tabSize) <= columnStartVal) break;

            if (oldCount > columnStartVal) t += s_tabSize;    
            else t += currColumnCount - columnStartVal;
        }
            break;
        default:

            if (t < m_screenWidth() && ++currColumnCount > columnStartVal)
            {
                m_setGrid(t, i, *it);
                
                ++t;
            }

            break;
        }
    }
}

void ConsoleTextEditor::m_deleteCharAt(const std::size_t index) noexcept
{
    const auto it = m_inputBuffer.begin() + index;

    if (*it == L'\n')
    {
        if (--m_rowCount < m_screenHeight() && m_startRow > 0)
        {	
            --m_startRow;
        }
    }
                    
    m_inputBuffer.erase(it);
}

void ConsoleTextEditor::m_insertChar(const wchar_t c) noexcept
{
    m_inputBuffer.insert(m_inputBuffer.begin() + m_currentIndex, c);

    ++m_currentIndex;
}

[[nodiscard]] constexpr std::size_t ConsoleTextEditor::m_getConsoleStartIndex() const noexcept
{
    std::size_t result = 0;

    if (m_startRow > 0)
    {
        std::size_t currentRow = 0;
        
        for (const auto& element :  m_inputBuffer)
        {
            ++result;

            if (element == L'\n')
            {
                if (m_startRow == ++currentRow) break;    
            }
        }

    }

    return result;
}

[[nodiscard]] constexpr std::size_t ConsoleTextEditor::m_getConsoleColumnStartIndex(const std::size_t consoleStartIndex) const noexcept
{
    std::size_t result = 0;

    for (std::size_t i = consoleStartIndex; i < m_currentIndex; ++i)
    {
        switch (m_inputBuffer.at(i))
        {
        case L'\n':
            result = 0;
            break;
        case L'\t':
            result += s_tabSize;
            break;
        default:
            ++result;
            break;
        }
    }

    const auto threshold = m_screenWidth() / 2;

    if(result >= threshold) return result - threshold + 1;

    return 0;
}

void ConsoleTextEditor::m_updateConsole() noexcept
{
    const auto consoleStartIndex = m_getConsoleStartIndex();

    m_clearConsole();

    m_updateScreenBuffer(consoleStartIndex);
    m_updateCursorPos   (consoleStartIndex);



    m_renderConsole();
}

void ConsoleTextEditor::m_updateCursorPos(const std::size_t consoleStartIndex) const noexcept
{
    COORD cursorPos = {};

    for (std::size_t i = consoleStartIndex; i < m_currentIndex; ++i)
    {
        switch (m_inputBuffer.at(i))
        {
        case L'\n':
            ++cursorPos.Y;
            cursorPos.X = 0;
            break;
        case L'\t':
            cursorPos.X += s_tabSize;
            break;
        default:
            ++cursorPos.X;
            break;
        }
    }

    const auto threshold = m_screenWidth() / 2;

    if(cursorPos.X >= threshold) cursorPos.X = static_cast<short>(threshold - 1);

    SetConsoleCursorPosition(m_getConsoleHandleOut(), cursorPos);
}


// [[nodiscard]] constexpr std::size_t m_findInBuffer(const wchar_t c, const std::size_t startIndex = 0) const noexcept
// {
//     std::size_t index = startIndex;

//     for (; index < m_inputBuffer.size(); ++index)
//     {
//         if (m_inputBuffer.at(index) == c) break;
//     }

//     return index;
// }