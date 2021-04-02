#include "console_text_editor.h"

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
		m_editors[Editor_Main].m_readFile(argv[1]);
		m_editors[Editor_Main].m_updateConsole(*this);
		m_renderConsole();
	}


	return true;
}

bool ConsoleTextEditor::m_childCreate()
{	
	return true;
}

void ConsoleTextEditor::m_childDestroy()
{
}

void ConsoleTextEditor::m_childUpdate(const float) 
{ 
	// std::wstringstream ss;

	// if (m_selectionInProgress) ss << L"Select ";

	// ss << L"bufferSize = " << m_inputBuffer.size() << L", bufferCapacity = " << m_inputBuffer.capacity() << L", m_currentIndex = " << m_currentIndex 
	// << L", m_rowCount = " << m_rowCount << L" m_startRow = " << m_startRow;

	// m_consoleTitle = std::move(ss.str());
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
			
				m_currentEditor = Editor_Save;
				break;
			case VirtualKeyCode::O:
				// open file event
				
				m_currentEditor = Editor_Open;
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
				// find next/previous event

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
				// open file

				if (m_editors[Editor_Main].m_readFile(m_editors[m_currentEditor].m_buffer()))
				{
					m_currentEditor = Editor_Main;
				}

				return;
			default:
				break;
			}

			break;
		default:
			break;
		}
	}

	if (!m_editors[m_currentEditor].m_handleEvents(event)) m_closeConsole();

	if (m_currentEditor == Editor_Find || m_currentEditor == Editor_Replace)
	{
		m_editors[Editor_Find   ].m_syncHeightWithRows(m_screenHeight());
		m_editors[Editor_Replace].m_syncHeightWithRows(m_editors[Editor_Find].m_drawStartY - 1);
	}

	m_updateConsole();
}

void ConsoleTextEditor::m_childHandleMouseEvents(const MOUSE_EVENT_RECORD&) {}
void ConsoleTextEditor::m_childHandleResizeEvent(const COORD, const COORD)
{	
	m_initEditors();
	
	if (m_currentEditor != Editor_Main) m_editors[Editor_Main].m_updateConsole(*this);

	m_updateConsole();

	m_setCursorPos({ 0, 0 });
}

void ConsoleTextEditor::m_initEditors() noexcept
{
	m_editors[Editor_Main   ].m_initEditor(m_screenWidth(), m_screenHeight());
	m_editors[Editor_Save   ].m_initEditor(m_screenWidth(), 1, s_openSaveEditorColor, 0, m_screenHeight() - 1);
	m_editors[Editor_Open   ].m_initEditor(m_screenWidth(), 1, s_openSaveEditorColor, 0, m_screenHeight() - 1);
	m_editors[Editor_Find   ].m_initEditor(m_screenWidth(), 4, s_openSaveEditorColor, 0, m_screenHeight() - 4);
	m_editors[Editor_Replace].m_initEditor(m_screenWidth(), 8, s_openSaveEditorColor, 0, m_screenHeight() - 8);

}

void ConsoleTextEditor::m_updateEditor(const EditorType editorT, const std::wstring_view header) noexcept
{
	m_drawString(m_editors[editorT].m_drawStartX, m_editors[editorT].m_drawStartY - 1, header, s_openSaveEditorColor, false);
	
	m_editors[editorT].m_updateConsole(*this);
}


void ConsoleTextEditor::m_updateConsole() noexcept
{	
	switch (m_currentEditor)
	{
	case Editor_Find:
	case Editor_Replace:
	case Editor_Main:
	
		m_editors[Editor_Main].m_updateConsole(*this, m_editors[Editor_Find].m_buffer());
		
		if (m_currentEditor != Editor_Main)
		{
			std::wstringstream ss;

			const auto [index, count] = m_editors[Editor_Main].m_getMatchResults(m_editors[Editor_Find].m_buffer());			
			
			ss << L"Find in editor: (" << index << L" of " << count << L")";

			m_updateEditor(Editor_Find   , ss.str());
			m_updateEditor(Editor_Replace, L"Replace in file");
		} 

		break;
	case Editor_Save:
		m_updateEditor(m_currentEditor, L"Save to File:");
		break;
	case Editor_Open:
		m_updateEditor(m_currentEditor, L"Open a File:");
		break;
	}
	
	m_renderConsole();
	m_setCursorPos(m_editors[m_currentEditor].m_cursorPos);
}
