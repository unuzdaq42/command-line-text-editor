#include "console_text_editor.h"

#include <sstream> // //

#define _WIN32_WINNT 0x0502 // // //

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

	return m_construct(width, height, fontW, fontH, true, s_defalutConsoleMode | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE);
}

bool ConsoleTextEditor::m_childConstruct()
{
	
	m_inputBuffer += 
	L"selamlar canisi selamlar\n"
	L"xd xd xd xd xd xd xd xd\n"
	L"selamlar canisi nabiyn\n"
	L"selamlar canısı xd\n"
	L"selamlar lol xd\n"
	L"xd xd xd xd xd xd xd\n"
	L"xd xd xd xd xd xd xd\n";
	m_rowCount = 7;
	

	m_inputBuffer.push_back(L' ');
	

	m_updateConsole();

	return true;
}

void ConsoleTextEditor::m_childUpdate(const float) 
{ 
	GetConsoleSelectionInfo(&m_consoleSI);

	if (m_consoleSI.dwFlags)
	{
		m_currentIndex = 0;
	}

	std::wstringstream ss;

	ss << L"m_currentIndex = " << m_currentIndex << L", m_rowCount = " << m_rowCount << L" m_startRow = " << m_startRow;


	// ss << L", X = " << csi.dwSelectionAnchor.X << L", Y = " << csi.dwSelectionAnchor.Y;
	// ss << L", Top = " << csi.srSelection.Top << L", Bottom = " << 
	// csi.srSelection.Bottom << L", Left = " << csi.srSelection.Left << L", Right = " << csi.srSelection.Right;


	

	m_consoleTitle = std::move(ss.str());
}

void ConsoleTextEditor::m_childHandleKeyEvents(const KEY_EVENT_RECORD event) 
{
	if (m_handleKeyEvents(event)) m_updateConsole();
}

void ConsoleTextEditor::m_childHandleMouseEvents(const MOUSE_EVENT_RECORD) {}
void ConsoleTextEditor::m_childHandleResizeEvent(const COORD, const COORD)
{
	m_updateConsole();
}

bool ConsoleTextEditor::m_handleKeyEvents(const KEY_EVENT_RECORD event)
{
	if (!event.bKeyDown) return false;

	const std::size_t startRowThreshold = m_screenHeight() / 2;

	switch (event.wVirtualKeyCode)
	{
	case VK_ESCAPE:
		m_closeConsole();
		break;
	case VK_BACK:

		if (m_deleteIfSelected()) return true;
		else if (m_currentIndex > 0)
		{
			m_deleteCharAt(--m_currentIndex);
			return true;
		}

		break;
	case VK_DELETE:

		if (m_deleteIfSelected()) return true;
		else if (m_inputBuffer.size() > m_currentIndex + 1)
		{
			m_deleteCharAt(m_currentIndex);
			return true;
		}

		break;
	case VK_LEFT:

		if (m_currentIndex > 0)
		{
			////////////////////////////////// BRUH
			if (m_inputBuffer.at(--m_currentIndex) == L'\n' && m_startRow > 0)
			{
				--m_startRow;
			}

			return true;
		}

		break;
	case VK_RIGHT:

		if (m_currentIndex + 1 < m_inputBuffer.size())
		{
			if (m_inputBuffer.at(m_currentIndex) == L'\n')
			{
				////////////////////////////////// BRUH
				if (m_getRowCountUntil(m_currentIndex) > startRowThreshold)
				{
					++m_startRow;
				}

			}

			++m_currentIndex;

			return true;
		}

		break;
	case VK_UP:
	{
		if (m_currentIndex == 0) return false;

		if (m_startRow > 0) --m_startRow;

		std::size_t colCount = 0;

		for (auto it = m_inputBuffer.crbegin() + m_inputBuffer.size() - m_currentIndex;
			it != m_inputBuffer.crend(); ++it)
		{
			if (*it == L'\n') break;

			++colCount;
		}

		if (m_currentIndex <= colCount + 1)
		{
			m_currentIndex = 0;
			return true;
		}

		const std::size_t firstNewLine = m_currentIndex - colCount - 1;
		std::size_t newLineIndex = firstNewLine;

		while (m_inputBuffer.at(--newLineIndex) != L'\n')
		{
			if (newLineIndex == 0)
			{
				m_currentIndex = colCount;
				return true;
			}
		}

		if (newLineIndex == firstNewLine - 1)
		{
			m_currentIndex = firstNewLine;
			return true;
		}

		std::size_t i = firstNewLine - 1;

		for (; i > newLineIndex + colCount + 1; --i)
		{
			if (m_inputBuffer.at(i) == L'\n') break;
		}

		m_currentIndex = i;

		return true;
	}
	case VK_DOWN:
	{
		////////////////////////////////// BRUH
		if (m_getRowCountUntil(m_currentIndex) > startRowThreshold)
		{
			++m_startRow;
		}

		std::size_t colCount = 0;

		for (auto it = m_inputBuffer.crbegin() + m_inputBuffer.size() - m_currentIndex;
			it != m_inputBuffer.crend(); ++it)
		{
			if (*it == L'\n') break;

			++colCount;
		}

		std::size_t newLineIndex = m_currentIndex;

		while (m_inputBuffer.at(newLineIndex++) != L'\n')
		{
			if (newLineIndex >= m_inputBuffer.size()) return false;
		}

		const std::size_t size = std::min(newLineIndex + colCount, m_inputBuffer.size() - 1);

		std::size_t i = newLineIndex;

		for (; i < size; ++i)
		{
			if (m_inputBuffer.at(i) == L'\n') break;
		}

		m_currentIndex = i;

		return true;
	}
	case VK_RETURN:

		m_insertChar(L'\n');

		if (++m_rowCount > startRowThreshold) ++m_startRow;

		return true;
	default:

		if (event.uChar.UnicodeChar)
		{
			m_insertChar(event.uChar.UnicodeChar);
			return true;
		}
		break;    
	}

	return false;
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

			m_drawPixel(t, i, true, false, false, false);

			if (++i >= m_screenHeight()) return;

			t = 0;
			currColumnCount = 0;
			
			break;
		case L'\t':
		{

			for (std::size_t j = 0; j < s_tabSize; ++j)
			{
				m_drawPixel(t + j, i, false, true, false, false);
			}

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


bool ConsoleTextEditor::m_deleteIfSelected() noexcept
{
	/*const bool val = GetConsoleSelectionInfo(&m_consoleSI);
	
	if (m_consoleSI.dwFlags || !val)
	{
		m_currentIndex = 42;
	}*/
	
	//if (m_consoleSI.dwFlags == CONSOLE_NO_SELECTION || (m_consoleSI.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) != CONSOLE_SELECTION_NOT_EMPTY) return false;

   /* const auto startInd    = m_getConsoleStartIndex();    
	const auto colStartInd = m_getConsoleColumnStartIndex(startInd);

	const std::size_t columnIndex = colStartInd + m_consoleSI.dwSelectionAnchor.X;

	std::size_t currentRow = 0;

	std::size_t index = startInd;

	for (; index < m_inputBuffer.size(); ++index)
	{
		if (m_inputBuffer.at(index) == L'\n')
		{
			if (++currentRow == m_startRow  + m_consoleSI.dwSelectionAnchor.Y) break;
		}
	}

	m_deleteCharAt(index + columnIndex);*/

	//m_currentIndex = 0;

	return false;
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

[[nodiscard]] constexpr std::size_t ConsoleTextEditor::m_getRowCountUntil(const std::size_t index) const noexcept
{
	std::size_t result = 1;

	for (std::size_t i = 0; i <= index; ++i)
	{
		if (m_inputBuffer.at(i) == L'\n') ++result;
	}

	return result;
}


void ConsoleTextEditor::m_updateConsole() noexcept
{
	const auto consoleStartIndex = m_getConsoleStartIndex();

	m_clearConsole();

	m_updateScreenBuffer(consoleStartIndex);
	m_updateCursorPos   (consoleStartIndex);

	m_renderConsole();
}




// TODO

// convert console to old state after done with it - done

// make default cursor workable its just sits there - done 

// change names - done?

// default max buffer constuctor

// down up arrows

// ctrl c, ctrl v

// void ConsoleTextEditor::m_deleteCharBetween(const std::size_t startInd, const std::size_t endInd) noexcept
// {
//     if (endInd <= startInd) return;

//     const auto startIt = m_inputBuffer.begin() + startInd;
//     const auto endIt   = m_inputBuffer.begin() + endInd  ;

//     for (auto it = startIt; it != endIt; ++it)
//     {
//         if (*it == L'\n')
//         {
//             if (--m_rowCount < m_screenHeight() && m_startRow > 0)
//             {	
//                 --m_startRow;
//             }
//         }
//     }

//     m_inputBuffer.erase(startIt, endIt);
// }

// [[nodiscard]] constexpr std::size_t m_findInBuffer(const wchar_t c, const std::size_t startIndex = 0) const noexcept
// {
//     std::size_t index = startIndex;

//     for (; index < m_inputBuffer.size(); ++index)
//     {
//         if (m_inputBuffer.at(index) == c) break;
//     }

//     return index;
// }