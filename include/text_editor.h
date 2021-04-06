#ifndef TEXT_EDITOR_H
#define TEXT_EDITOR_H

#include <type_traits>
#include <variant>
#include <deque>

#include "console.h"
#include "utility.h"

class TextEditor
{
public:

    using SizeType = std::wstring::size_type;

    void m_initEditor(const SizeType width, const SizeType height, 
        const WORD textColor = Console::s_foregroundWhite, 
        const SizeType startX = 0, const SizeType startY = 0);

    bool m_handleEvents(const Console& console, const KEY_EVENT_RECORD  & event);
    void m_handleEvents(const Console& console, const MOUSE_EVENT_RECORD& event);

    void m_updateConsole(Console& console, const std::wstring_view searchStr = {}) noexcept;

    void m_syncHeightWithRows(const SizeType consoleHeight) noexcept;
    

    [[nodiscard]] std::wstring_view m_buffer() const noexcept { return { m_inputBuffer.c_str(), m_inputBuffer.size() - 1 }; }

    [[nodiscard]] constexpr bool m_isInsidePoint(const SizeType x, const SizeType y) const noexcept
    {   
        return x >= m_drawStartX && y >= m_drawStartY && x < m_drawStartX + m_width && y < m_drawStartY + m_height;
    }

    [[nodiscard]] constexpr bool m_isStringSelected() const noexcept { return m_selectionInProgress; }

    [[nodiscard]] std::optional<std::wstring_view> m_getSelectedStr() const noexcept 
    { 
        if (!m_selectionInProgress) return {};

        const auto[min, max] = utils::GetMinMax(m_currentIndex, m_selectionStartIndex);

        return { { m_inputBuffer.c_str() + min, max - min + 1 } }; 
    }

    bool m_selectNextString    (const std::wstring_view str) noexcept;
    bool m_selectPreviousString(const std::wstring_view str) noexcept;

    void m_setInputBuffer      (const std::wstring_view str) noexcept;

    bool m_readFile            (const std::wstring_view filePath) noexcept;
    bool m_writeFile           (const std::wstring_view filePath) const noexcept;

    [[nodiscard]] std::pair<SizeType, SizeType> m_getMatchResults(const std::wstring_view str) const noexcept;

public:

    SizeType m_drawStartX = 0;
    SizeType m_drawStartY = 0;

    SizeType m_width  = 0;
    SizeType m_height = 0;

    COORD m_cursorPos = {};

private:

    constexpr void m_handleSelection(const SizeType start) noexcept;
    constexpr void m_handleSelection(const SizeType start, const SizeType end) noexcept;

    WORD m_textColor = 0;
    bool m_shiftPressed = false;

    enum class EventType
    {
        Keyboard,
        MouseButton,
        MouseWheel,
        None
    };

    EventType m_lastEvent = EventType::None;

private:

    std::wstring m_inputBuffer;

    SizeType m_currentIndex = 0;
    SizeType m_startRow = 0;

    SizeType m_rowCount = 1;
    
    bool m_selectionInProgress = false;
    SizeType m_selectionStartIndex = 0;

    static constexpr SizeType s_tabSize = 4;
    
private:

    void m_handleInsertionEvents (const KEY_EVENT_RECORD& event);
    void m_handleControlKeyEvents(const KEY_EVENT_RECORD& event);
    void m_handleCursorEvents    (const KEY_EVENT_RECORD& event);
    void m_handleCursorMoveEvents(const KEY_EVENT_RECORD& event);

    void m_moveCursorOneWordLeft () noexcept;
    void m_moveCursorOneWordRight() noexcept;  

    void m_moveCursorOneLineDown() noexcept;
    void m_moveCursorOneLineUp  () noexcept;

    void m_deleteCharAt(const SizeType index) noexcept;
    bool m_deleteIfSelected() noexcept;
    void m_deleteStartingFrom(const SizeType start, SizeType end) noexcept; 

    void m_insertChar(const wchar_t c) noexcept;
    void m_insertUnsafeString(std::wstring str);
    void m_insertString(const std::wstring_view str, const SizeType insertIndex);

    constexpr void m_scrollOneUp  () noexcept { if (m_startRow > 0) --m_startRow; }
    constexpr void m_scrollOneDown() noexcept { if (m_startRow + m_height < m_rowCount) ++m_startRow; }

public:

    void m_insertString(const std::wstring_view str);
    void m_replaceMatchsWith(const std::wstring_view keyStr, const std::wstring_view replaceStr) noexcept;

private:
    
    struct InsertionRecord
    {
        SizeType m_index;
        SizeType m_size;
    };

    struct DeletionRecord
    {
        SizeType m_index;
        std::wstring m_data;
    };

private:

    std::deque<std::variant<InsertionRecord, DeletionRecord>> m_records;

    void m_writeInsertionRecord(const SizeType index, const SizeType size, const bool createNew = true) noexcept;
    void m_writeDeletionRecord (const SizeType index, std::wstring&& str , const bool createNew = true) noexcept;

    void m_resizeRecordsIfNeeded() noexcept;

private:

    [[nodiscard]] SizeType m_getIndexAtPos(const SizeType x, const SizeType y) const noexcept;

private:

    constexpr void m_updateStartRow() noexcept;

    // returns top left pixels index value ( according to m_inputBuffer )
    [[nodiscard]] constexpr SizeType m_getConsoleStartIndex() const noexcept;

    [[nodiscard]] constexpr SizeType m_getConsoleColumnStartIndex(const SizeType consoleStartIndex) const noexcept;

    [[nodiscard]] constexpr SizeType m_getRowCountUntil(const SizeType index) const noexcept;


};



#endif