#ifndef CONSOLE_TEXT_EDITOR_H
#define CONSOLE_TEXT_EDITOR_H

#include <array>

#include "text_editor.h"


class ConsoleTextEditor : public Console
{
public:

    [[nodiscard]] bool m_constructEditor(const int argc, const wchar_t* argv[]) noexcept;

private:

    static constexpr std::size_t s_editorCount = 5;

    enum EditorType : std::size_t 
    {
        Editor_Main,
        Editor_Save,
        Editor_Open,
        Editor_Find,
        Editor_Replace
    };

    std::array<TextEditor, s_editorCount> m_editors;

    EditorType m_currentEditor = Editor_Main;

    void m_initEditors() noexcept;

    void m_updateEditors() noexcept;

    void m_updateEditor(const EditorType editorT, const std::wstring_view header) noexcept;

private:

    static constexpr WORD s_openSaveEditorColor = s_foregroundWhite | BACKGROUND_RED | BACKGROUND_BLUE;

private:

    void m_childHandleKeyEvents  (const KEY_EVENT_RECORD&  ) final override;
    void m_childHandleMouseEvents(const MOUSE_EVENT_RECORD&) final override;
	void m_childHandleResizeEvent(const COORD, const COORD ) final override;
};




#endif