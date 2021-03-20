#ifndef CONSOLE_TEXT_EDITOR_H
#define CONSOLE_TEXT_EDITOR_H

#include "console.h"

class ConsoleTextEditor : public Console
{
public:

    [[nodiscard]] bool m_constructEditor(const int argc, const char* argv[]) noexcept;

private:

    bool m_childConstruct() override;

    void m_childUpdate(const float) override;

    void m_childHandleKeyEvents  (const KEY_EVENT_RECORD  ) override;
    void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD) override;
	void m_childHandleResizeEvent(const COORD, const COORD) override;

private:

    std::wstring m_inputBuffer;

    std::size_t m_currentIndex = 0;

    std::size_t m_rowCount = 1;
    std::size_t m_startRow = 0;

    static constexpr std::size_t s_tabSize = 4;

private:

    bool m_handleKeyEvents(const KEY_EVENT_RECORD event);

    void m_updateCursorPos   (const std::size_t consoleStartIndex) const noexcept;
    void m_updateScreenBuffer(const std::size_t consoleStartIndex) noexcept;
    void m_updateConsole() noexcept;

    void m_deleteCharAt(const std::size_t index) noexcept;

    void m_insertChar(const wchar_t c) noexcept;

    // returns top left pixels index value ( according to m_inputBuffer )
    [[nodiscard]] constexpr std::size_t m_getConsoleStartIndex() const noexcept;

    [[nodiscard]] constexpr std::size_t m_getConsoleColumnStartIndex(const std::size_t consoleStartIndex) const noexcept;

    [[nodiscard]] constexpr std::size_t m_getRowCountUntil(const std::size_t index) const noexcept;
};



#endif