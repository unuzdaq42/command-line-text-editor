#include "../include/console_text_editor.h"

#include <sstream>


[[nodiscard]] bool ConsoleTextEditor::m_constructEditor(const int argc, const wchar_t* argv[]) noexcept
{	

	int width  = 80;
	int height = 40;
	
	short fontW = 8;
	short fontH = 16;


	if (argc > 3)
	{
		width  = _wtoi(argv[2]);
		height = _wtoi(argv[3]);
	
		if (argc > 5)
		{
			fontW = static_cast<short>(_wtoi(argv[4]));
			fontH = static_cast<short>(_wtoi(argv[5]));
		}
	}
	
	if (!m_construct(width, height, fontW, fontH, true, s_defalutConsoleMode )) return false;

	m_initEditors();
	
	if (argc > 1)
	{
		const std::wstring_view str = argv[1];

		if (m_editors[Editor_Main].m_readFile(str))
		{
			m_updateEditors();
			m_setConsoleTitle(utils::GetFileName(str));
		}	
	}
	else
	{
		m_setConsoleTitle(L"Untitled");
	}

	return true;
}

void ConsoleTextEditor::m_childHandleKeyEvents(const KEY_EVENT_RECORD& event) 
{
	// handle editor change events
	if (event.bKeyDown)
	{
		if (s_isCtrlKeyPressed(event))
		{
			// control key events

			switch (event.wVirtualKeyCode)
			{
			case VirtualKeyCode::S:
				// save file event
				if (m_currentEditor == Editor_Main) m_currentEditor = Editor_Save;

				break;
			case VirtualKeyCode::O:
				// open file event
				if (m_currentEditor == Editor_Main) m_currentEditor = Editor_Open;
				
				break;
			case VirtualKeyCode::F:
			{
				// find event

				if (m_currentEditor == Editor_Main)
				{
					const auto str = m_editors[Editor_Main].m_getSelectedStr();

					if (str.has_value())
					{
						m_editors[Editor_Find].m_setInputBuffer(str.value());
					}
				}

				m_currentEditor = Editor_Find;
				break;
			}
			case VirtualKeyCode::H:
				// replace event

				m_currentEditor = Editor_Replace;
				break;
			case VK_RETURN:
				// find/replace next/previous event

				if (m_currentEditor == Editor_Replace && s_isAltKeyPressed(event))
				{
					m_editors[Editor_Main].m_replaceMatchsWith(m_editors[Editor_Find].m_buffer(), m_editors[Editor_Replace].m_buffer());
				}
				else if (m_currentEditor == Editor_Find || m_currentEditor == Editor_Replace)
				{
					if (s_isShiftKeyPressed(event))
					{
						m_editors[Editor_Main].m_selectPreviousString(m_editors[Editor_Find].m_buffer());
					}
					else
					{
						m_editors[Editor_Main].m_selectNextString(m_editors[Editor_Find].m_buffer());
					}

					if (m_currentEditor == Editor_Replace && m_editors[Editor_Main].m_isStringSelected())
					{
						m_editors[Editor_Main].m_insertString(m_editors[Editor_Replace].m_buffer());
					}
				}

				break;
			default:
				break;
			}

		}

		switch (event.wVirtualKeyCode)
		{
		
		case VK_ESCAPE:

			if (m_currentEditor != Editor_Main)
			{
				m_currentEditor = Editor_Main;
				m_editors[Editor_Main].m_height = m_screenHeight();
				return;
			}

			break;
		case VK_RETURN:

			switch (m_currentEditor)
			{
			case Editor_Save:
				// save file
				if (m_editors[Editor_Main].m_writeFile(m_editors[m_currentEditor].m_buffer()))
				{
					m_currentEditor = Editor_Main;
				}

				return;
			case Editor_Open:
			{
				// open file

				const auto str = m_editors[m_currentEditor].m_buffer();

				if (m_editors[Editor_Main].m_readFile(str))
				{
					m_setConsoleTitle(utils::GetFileName(str));
					m_currentEditor = Editor_Main;
				}

				return;
			}
			default:
				break;
			}

			break;
		default:
			break;
		}
	}

	if (!m_editors[m_currentEditor].m_handleEvents(*this, event)) m_closeConsole();

	if (m_currentEditor == Editor_Find || m_currentEditor == Editor_Replace)
	{
		m_editors[Editor_Find   ].m_syncHeightWithRows(m_screenHeight());
		m_editors[Editor_Replace].m_syncHeightWithRows(m_editors[Editor_Find].m_drawStartY - 1);

		m_editors[Editor_Main].m_height = m_editors[Editor_Replace].m_drawStartY - 1;
	}

	m_updateEditors();

	m_setCursorPos(m_editors[m_currentEditor].m_cursorPos);
}

void ConsoleTextEditor::m_childHandleMouseEvents(const MOUSE_EVENT_RECORD& event) 
{
	if (s_isLeftButtonPressed(event))
	{
		auto isInsidePoint = [&] (const EditorType type)
		{
			return m_editors[type].m_isInsidePoint(event.dwMousePosition.X, event.dwMousePosition.Y);
		};

		switch (m_currentEditor)
		{
		case Editor_Main:
			break;
		case Editor_Save:
		case Editor_Open:
			if (!isInsidePoint(m_currentEditor))
			{
				m_currentEditor = Editor_Main;
			}

			break;
		case Editor_Find:
		case Editor_Replace:

			if (isInsidePoint(Editor_Find))
			{
				m_currentEditor = Editor_Find;
			}
			else if (isInsidePoint(Editor_Replace))
			{
				m_currentEditor = Editor_Replace;
			}
			else
			{
				m_currentEditor = Editor_Main;
			}
			break;
		}
	}

	m_editors[m_currentEditor].m_handleEvents(*this, event);

	m_updateEditors();
	m_setCursorPos(m_editors[m_currentEditor].m_cursorPos);

}

void ConsoleTextEditor::m_childHandleResizeEvent(const COORD, const COORD)
{	
	m_initEditors();
	
	m_updateEditors();
	
	// windows shows scrollbar when this is Editor_Find or Editor_Replace
	// on resize events probably due to cursor position
	m_setCursorPos({ 0, 0 });
}

void ConsoleTextEditor::m_initEditors() noexcept
{
	m_editors[Editor_Main   ].m_initEditor(m_screenWidth(), m_screenHeight());
	m_editors[Editor_Save   ].m_initEditor(m_screenWidth(), 2, s_openSaveEditorColor, 0, m_screenHeight() - 2);
	m_editors[Editor_Open   ].m_initEditor(m_screenWidth(), 2, s_openSaveEditorColor, 0, m_screenHeight() - 2);
	m_editors[Editor_Find   ].m_initEditor(m_screenWidth(), 4, s_openSaveEditorColor, 0, m_screenHeight() - 4);
	m_editors[Editor_Replace].m_initEditor(m_screenWidth(), 8, s_openSaveEditorColor, 0, m_screenHeight() - 8);

}

void ConsoleTextEditor::m_updateEditor(const EditorType editorT, const std::wstring_view header) noexcept
{
	m_drawString(m_editors[editorT].m_drawStartX, m_editors[editorT].m_drawStartY - 1, header, s_openSaveEditorColor, false);
	
	m_editors[editorT].m_updateConsole(*this);
}


void ConsoleTextEditor::m_updateEditors() noexcept
{	
	m_clearConsole();

	m_editors[Editor_Main].m_updateConsole(*this, m_editors[Editor_Find].m_buffer());

	switch (m_currentEditor)
	{
	case Editor_Find:
	case Editor_Replace:
	{
		std::wstringstream ss;

		const auto [index, count] = m_editors[Editor_Main].m_getMatchResults(m_editors[Editor_Find].m_buffer());			
		
		ss << L"Find in editor: (" << index << L" of " << count << L")";

		m_updateEditor(Editor_Find   , ss.str());
		m_updateEditor(Editor_Replace, L"Replace in file");
		
		break;
	}
	case Editor_Save:
		m_updateEditor(m_currentEditor, L"Save to File:");
		break;
	case Editor_Open:
		m_updateEditor(m_currentEditor, L"Open a File:");
		break;
	case Editor_Main:
		break;
	}
	
	m_renderConsole();	
}



