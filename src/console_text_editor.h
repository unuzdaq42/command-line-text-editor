#ifndef CONSOLE_TEXT_EDITOR_H
#define CONSOLE_TEXT_EDITOR_H

#include "console.h"

#include <utility>
#include <string_view>
#include <optional>

/*

    keep in mind

    seperate logic and windows functions

    DONE

    replace m_getConsoleHandleOut() with m_setCursorPos or whatever

    perhaps no need for m_updateCursorPos function
    because m_updateScreenBuffer finds cursorPos anyways 

    typedef a index type ( probably unsigned int )
    std::size_t spam doesnt look good
    
    right alt key doesnt work --> could be better

    TODO:

    m_handleKeyEvents VK_BACK and VK_DELETE maybe no need for another m_deleteCharAt function
    selection could be default and it handles everything

    changing font type, size at runtime by user
    user colors maybe ( maybe pen color kind of thing )

    file system input, output

    you dont have to do all of them but:
    mouse selection, mouse wheel scroll
    select line, select all, select word, ctrl + arrow keys, find and replace, find next 

    maybe color on find and replace

    make constants such as startRowThreshold, column threshold

    debug release option on CONSOLE_ASSERT

    BUGS

    if m_inputBuffer is "\n " and m_currentIndex = 1 user cannot go one line up

*/
class ConsoleTextEditor : public Console
{
public:

    [[nodiscard]] bool m_constructEditor(const int argc, const char* argv[]) noexcept;

private:

    bool m_childConstruct() override;
    void m_childDeconstruct() override;
    void m_childUpdate(const float) override;

    void m_childHandleKeyEvents  (const KEY_EVENT_RECORD&  ) override;
    void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD&) override;
	void m_childHandleResizeEvent(const COORD, const COORD ) override;

private:

    using IndexType = std::size_t;

    std::wstring m_inputBuffer;

    IndexType m_currentIndex = 0;

    IndexType m_rowCount = 1;
    IndexType m_startRow = 0;

    static constexpr IndexType s_tabSize = 4;
    [[nodiscard]] constexpr IndexType m_verticalScrollThreshold  () const noexcept { return m_screenHeight() / 2; }
    [[nodiscard]] constexpr IndexType m_horizontalScrollThreshold() const noexcept { return m_screenWidth () / 2; }

    bool m_selectionInProgress = false;
    IndexType m_selectionStartIndex = 0;

private:

    static constexpr auto s_ctrlKeyFlag = RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED;
    // constexpr auto altgr = (RIGHT_ALT_PRESSED | LEFT_CTRL_PRESSED);
private:

    void m_handleKeyEvents(const KEY_EVENT_RECORD& event);
    void m_handleSelectionEvent(const KEY_EVENT_RECORD& event, 
        const IndexType oldInputIndex, const std::size_t oldBufferSize) noexcept;
    void m_handleControlKeyEvents(const KEY_EVENT_RECORD& event) noexcept;

    void m_updateScreenBuffer() noexcept;
    void m_updateConsole() noexcept;

    void m_deleteCharAt(const IndexType index) noexcept;
    bool m_deleteIfSelected() noexcept;

    void m_scrollDownIfNeeded() noexcept;
    void m_handleCharDeletion(const wchar_t c) noexcept;

    void m_insertChar(const wchar_t c) noexcept;

    // returns top left pixels index value ( according to m_inputBuffer )
    [[nodiscard]] constexpr IndexType m_getConsoleStartIndex() const noexcept;

    [[nodiscard]] constexpr IndexType m_getConsoleColumnStartIndex(const IndexType consoleStartIndex) const noexcept;

    [[nodiscard]] constexpr IndexType m_getRowCountUntil(const IndexType index) const noexcept;

    [[nodiscard]] static constexpr std::pair<IndexType, IndexType> 
    s_getMinMax(const IndexType first, const IndexType second) noexcept
    {
        if (first > second)
        {
            return { second, first };
        }

        return { first, second };
    }

    [[nodiscard]] static constexpr IndexType s_getMin(const IndexType first, const IndexType second) noexcept
    {
        return (first < second) ? first : second;
    }   

    // not used
    // static constexpr IndexType s_getMax(const IndexType first, const IndexType second) noexcept
    // {
    //     return (first > second) ? first : second;
    // }   


    static bool s_setUserClipboard(const std::wstring_view str) noexcept;
    [[nodiscard]] static std::optional<std::wstring> s_getUserClipboard() noexcept;


    [[nodiscard]] static bool s_isCtrlKeyPressed(const DWORD flag) noexcept
    {
        // windows gives ctrl left event when user presses alt gr key
        // couldnt find anything better than this 
        const bool isAltGrKeyPressed = GetKeyState(VK_RMENU) & 0x8000;

        return (flag & s_ctrlKeyFlag) != 0 && !isAltGrKeyPressed;
    }
};



#endif