#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include "console.h"

#include <utility>
#include <string_view>
#include <type_traits>

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

    make constants such as startRowThreshold, column threshold

    TODO:

    control Z ( change history )

    m_handleKeyEvents VK_BACK and VK_DELETE maybe no need for another m_deleteCharAt function
    selection could be default and it handles everything

    changing font type, size at runtime by user
    user colors maybe ( maybe pen color kind of thing )

    file system input, output

    you dont have to do all of them but:
    mouse selection, mouse wheel scroll
    find and replace, find next 

    maybe color on find and replace

    debug release option on CONSOLE_ASSERT

    BUGS

    control window close event

*/

class TextEditor
{
public:

    using IndexType = std::size_t;

    void m_initEditor(const IndexType width, const IndexType height, 
        const WORD textColor = Console::s_foregroundWhite, 
        const IndexType startX = 0, const IndexType startY = 0);

    bool m_handleEvents(const KEY_EVENT_RECORD& event);

    void m_updateConsole(Console& console, const std::wstring_view searchStr = {}) noexcept;

    void m_syncHeightWithRows(const IndexType consoleHeight) noexcept;
    

    [[nodiscard]] std::wstring_view   m_buffer   () const noexcept { return { m_inputBuffer.c_str(), m_inputBuffer.size() - 1 }; }
    [[nodiscard]] const std::wstring& m_refBuffer() const noexcept { return m_inputBuffer; }

    [[nodiscard]] constexpr bool m_isStringSelected() const noexcept { return m_selectionInProgress; }

    [[nodiscard]] std::optional<std::wstring_view> m_getSelectedStr() const noexcept 
    { 
        if (!m_selectionInProgress) return {};

        const auto[min, max] = s_getMinMax(m_currentIndex, m_selectionStartIndex);

        return { { m_inputBuffer.c_str() + min, max - min + 1 } }; 
    }

    [[nodiscard]] std::pair<std::size_t, std::size_t> m_getMatchResults(const std::wstring_view str) const noexcept;

    bool m_selectNextString    (const std::wstring_view str) noexcept;
    bool m_selectPreviousString(const std::wstring_view str) noexcept;

    void m_setInputBuffer      (const std::wstring_view str) noexcept;

    bool m_readFile            (const std::wstring_view filePath) noexcept;
    bool m_writeFile           (const std::wstring_view filePath) const noexcept;

public:

    IndexType m_drawStartX = 0;
    IndexType m_drawStartY = 0;

    IndexType m_width  = 0;
    IndexType m_height = 0;

    COORD m_cursorPos = {};

private:

    constexpr void m_handleSelection(const IndexType start) noexcept;
    constexpr void m_handleSelection(const IndexType start, const IndexType end) noexcept;

    IndexType m_verScrollThreshold;
    IndexType m_horScrollThreshold;

    WORD m_textColor = 0;
    

private:

    std::wstring m_inputBuffer;

    IndexType m_currentIndex = 0;

    IndexType m_rowCount = 1;
    IndexType m_startRow = 0;
    
    bool m_selectionInProgress = false;
    IndexType m_selectionStartIndex = 0;

    static constexpr IndexType s_tabSize = 4;
 
private:

    bool m_handleKeyEvents       (const KEY_EVENT_RECORD& event);
    void m_handleControlKeyEvents(const KEY_EVENT_RECORD& event);
    void m_handleCursorEvents    (const KEY_EVENT_RECORD& event);
    void m_handleCursorMoveEvents(const KEY_EVENT_RECORD& event);

    void m_scrollDownIfNeeded() noexcept;
    
    void m_moveCursorOneWordLeft () noexcept;
    void m_moveCursorOneWordRight() noexcept;  

    void m_handleCharDeletion(const wchar_t c) noexcept;
    void m_deleteCharAt(const IndexType index) noexcept;
    bool m_deleteIfSelected() noexcept;

    void m_insertChar(const wchar_t c) noexcept;
    void m_insertUnsafeString(std::wstring str);

public:

    void m_insertString(const std::wstring_view str);
    void m_replaceMatchsWith(const std::wstring_view keyStr, const std::wstring_view replaceStr) noexcept;

private:

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

    [[nodiscard]] static constexpr auto s_getSignedDiff(const IndexType lhs, const IndexType rhs) noexcept
    {
        using type = std::make_signed<IndexType>::type;
        return static_cast<type>(lhs) - static_cast<type>(rhs);
    }
    
};



#endif