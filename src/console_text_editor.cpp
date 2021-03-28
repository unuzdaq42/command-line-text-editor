#include "console_text_editor.h"

#include <sstream>
#include <cwctype> // std::iswprint
#include <algorithm>
#include <cwchar>


[[nodiscard]] bool ConsoleTextEditor::m_constructEditor(const int argc, const wchar_t* argv[]) noexcept
{
	int width  = 100;
	int height = 50;
	
	short fontW = 8;
	short fontH = 16;

	if (argc > 1)
	{
		m_readFile(argv[1]);
	
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
	}


	return m_construct(width, height, fontW, fontH, true, s_defalutConsoleMode );
}

bool ConsoleTextEditor::m_childCreate()
{	
	m_inputBuffer.push_back(L' ');
	
	m_updateConsole();

	return true;
}

void ConsoleTextEditor::m_childDestroy()
{
}

void ConsoleTextEditor::m_childUpdate(const float) 
{ 
	
	std::wstringstream ss;

	if (m_selectionInProgress) ss << L"Select ";

	ss << L"bufferSize = " << m_inputBuffer.size() << L", bufferCapacity = " << m_inputBuffer.capacity() << L", m_currentIndex = " << m_currentIndex 
	<< L", m_rowCount = " << m_rowCount << L" m_startRow = " << m_startRow;

	m_consoleTitle = std::move(ss.str());
}

void ConsoleTextEditor::m_childHandleKeyEvents(const KEY_EVENT_RECORD& event) 
{
	const auto oldInputIndex = m_currentIndex;

	m_handleCursorMoveEvents(event);

	m_handleSelectionEvent(event, oldInputIndex);

	m_handleKeyEvents(event);

	m_handleControlKeyEvents(event);

 	m_updateConsole();
}

void ConsoleTextEditor::m_childHandleMouseEvents(const MOUSE_EVENT_RECORD&) {}
void ConsoleTextEditor::m_childHandleResizeEvent(const COORD, const COORD)
{
	m_updateConsole();
}

void ConsoleTextEditor::m_handleKeyEvents(const KEY_EVENT_RECORD& event)
{	
	if (!event.bKeyDown || s_isCtrlKeyPressed(event)) return;

	switch (event.wVirtualKeyCode)
	{
	case VK_ESCAPE:

		if (m_selectionInProgress) 
		{ 
			// cancel selection
			m_selectionInProgress = false; 
			m_scrollDownIfNeeded(); 
		}
		else m_closeConsole(); // close editor

		break;
	case VK_BACK:
	
		if (!m_deleteIfSelected() && m_currentIndex > 0)
		{
			m_deleteCharAt(--m_currentIndex);
		}

		break;
	case VK_DELETE:

		if (!m_deleteIfSelected() && m_currentIndex + 1 < m_inputBuffer.size())
		{
			m_deleteCharAt(m_currentIndex);
		}

		break;
	case VK_RETURN:

		m_deleteIfSelected();

		m_insertChar(L'\n');

		++m_rowCount;

		m_scrollDownIfNeeded();

		break;
	default:

		if (event.uChar.UnicodeChar)
		{
			m_deleteIfSelected();

			m_insertChar(event.uChar.UnicodeChar);
		}

		break;    
	}

}

void ConsoleTextEditor::m_handleSelectionEvent(const KEY_EVENT_RECORD& event, const IndexType oldInputIndex) noexcept
{
	static bool s_shiftPressed = false;

	if (s_isShiftKeyEvent(event))
	{
		s_shiftPressed = event.bKeyDown;
	}

	if (oldInputIndex != m_currentIndex)
	{
		// user moved the cursor

		if (s_shiftPressed && !m_selectionInProgress)
		{
			m_selectionStartIndex = oldInputIndex;

			const auto diff = s_getSignedDiff(m_currentIndex, oldInputIndex);

			if (diff == 1 || diff == -1)
			{
				m_currentIndex -= diff;
			}
		}

		m_selectionInProgress = s_shiftPressed;
	}
}

void ConsoleTextEditor::m_handleControlKeyEvents(const KEY_EVENT_RECORD& event) noexcept
{
	if (!event.bKeyDown || !s_isCtrlKeyPressed(event)) return;

	switch (event.wVirtualKeyCode)
	{
	case VirtualKeyCode::C:
	case VirtualKeyCode::X:
	{
		// Copy and Cut event

		if (m_selectionInProgress)
		{
			// i see a lot of these
			const auto [min, max] = s_getMinMax(m_currentIndex, m_selectionStartIndex);
			
			// check errors?
			s_setUserClipboard({ m_inputBuffer.c_str() + min, max - min + 1 });

			if (event.wVirtualKeyCode == VirtualKeyCode::X)
			{
				// delete selected part if it is a cut event
				m_deleteIfSelected();
			}
		}
	}
		break;
	case VirtualKeyCode::V:
	{
		// Paste Event
		auto clipboard = s_getUserClipboard();

		if (clipboard.has_value())
		{
			m_deleteIfSelected();

			auto& str = clipboard.value();

			// preprocess string before putting it to input buffer
			for (auto it = str.cbegin(); it != str.cend();)
			{
				const auto element = *it;

				switch (element)
				{
				case L'\n':
					++m_rowCount;
					[[fallthrough]];
				case L'\t':
					++it;
					break;
				default:

					if (!std::iswprint(element))
					{
						// delete element if it is not printable and it is not accepted
						// as a control character
						it = str.erase(it);
					}
					else ++it;

					break;
				}
			}

			const auto secondPartSize = m_inputBuffer.size() - m_currentIndex;

			m_inputBuffer.resize(m_inputBuffer.size() + str.size());
			
			for (std::size_t i = 0; i < secondPartSize; ++i)
			{
				const auto index = m_currentIndex + (secondPartSize - i - 1);

				m_inputBuffer.at(index + str.size()) = m_inputBuffer.at(index);
			}
			
			for (std::size_t i = 0; i < str.size(); ++i)
			{
				m_inputBuffer.at(i + m_currentIndex) = str.at(i);
			}

			m_currentIndex += str.size();

			m_scrollDownIfNeeded();

		}
	}
		break;
	case VirtualKeyCode::A:
		// Select All Event

		m_selectionStartIndex = 0;

		m_currentIndex = m_inputBuffer.size() - 1;

		if (m_currentIndex > 0) --m_currentIndex; 

		m_selectionInProgress = true;

		break;
	case VirtualKeyCode::L:
	{
		// Select Line Event

		auto findStart = m_currentIndex;

		if (findStart > 0) --findStart;

		const auto start = m_inputBuffer.rfind(L'\n', findStart		);
		const auto end   = m_inputBuffer.find (L'\n', m_currentIndex);

		if (start == std::wstring::npos) m_currentIndex = 0;
		else m_currentIndex = start + 1;

		m_selectionStartIndex = std::min(end, m_inputBuffer.size() - 1);

		m_selectionInProgress = true;

		break;
	}
	case VirtualKeyCode::S:
		// Save to file event

		m_writeFile(L"c:\\users\\acer\\desktop\\someDeal.txt");

		break;
	case VK_BACK:
	case VK_DELETE:
	{
		// Delete Word Events

		if (m_deleteIfSelected()) break;

		auto oldInputIndex = m_currentIndex;

		if (event.wVirtualKeyCode == VK_BACK) m_moveCursorOneWordLeft();
		else m_moveCursorOneWordRight();

		if (m_currentIndex < oldInputIndex) --oldInputIndex;
		
		m_selectionStartIndex = oldInputIndex;
		m_selectionInProgress = true;

		m_deleteIfSelected();

		break;
	}
	default:
		break;
	}
}


void ConsoleTextEditor::m_handleCursorMoveEvents(const KEY_EVENT_RECORD& event) noexcept
{
	if (!event.bKeyDown) return;

	if (s_isCtrlKeyPressed(event))
	{
		// control key + movement events
		switch (event.wVirtualKeyCode)
		{
		case VK_LEFT:
		
			m_moveCursorOneWordLeft();
			break;
		case VK_RIGHT:
			m_moveCursorOneWordRight();
			break;
		case VK_HOME:
			
			m_currentIndex = 0;
			break;
		case VK_END:
			
			m_currentIndex = m_inputBuffer.size() - 1;
			break;
		default:
			break;
		}

		return;
	}


	switch (event.wVirtualKeyCode)
	{
	case VK_LEFT:

		if (m_currentIndex > 0)
		{
			if (m_inputBuffer.at(--m_currentIndex) == L'\n' && m_startRow > 0)
			{
				--m_startRow;
			}
		}

		break;
	case VK_RIGHT:

		if (m_currentIndex + 1 < m_inputBuffer.size())
		{
			if (m_inputBuffer.at(m_currentIndex) == L'\n')
			{
				m_scrollDownIfNeeded();
			}

			++m_currentIndex;
		}

		break;
	case VK_UP:
	{
		if (m_currentIndex == 0) break;

		const auto firstNewLineIndex = m_inputBuffer.rfind(L'\n', m_currentIndex - 1);

		if (firstNewLineIndex == 0) { m_currentIndex = 0; break; }
		if (firstNewLineIndex == std::wstring::npos) break;

		const auto columnCount = m_currentIndex - firstNewLineIndex;
		const auto secondNewLineIndex = m_inputBuffer.rfind(L'\n', firstNewLineIndex - 1);

		if (secondNewLineIndex == std::wstring::npos)
		{
			m_currentIndex = std::min(firstNewLineIndex, columnCount - 1);
		}
		else if (secondNewLineIndex == firstNewLineIndex - 1)
		{
			m_currentIndex = firstNewLineIndex;
		}
		else
		{
			m_currentIndex = std::min(firstNewLineIndex, secondNewLineIndex + columnCount);
		}

		if (m_startRow > 0) --m_startRow;

		break;
	}
	case VK_DOWN:
	{
		IndexType colCount = 0;

		for (auto it = m_inputBuffer.crend() - m_currentIndex; it != m_inputBuffer.crend(); ++it)
		{
			if (*it == L'\n') break;

			++colCount;
		}

		auto newLineIndex = m_currentIndex;

		while (m_inputBuffer.at(newLineIndex++) != L'\n')
		{
			if (newLineIndex >= m_inputBuffer.size()) return;
		}

		const auto size = s_getMin(newLineIndex + colCount, m_inputBuffer.size() - 1);

		auto i = newLineIndex;

		for (; i < size; ++i)
		{
			if (m_inputBuffer.at(i) == L'\n') break;
		}

		m_currentIndex = i;

		// dont scroll down if it is failed to go down one line
		m_scrollDownIfNeeded();

		break;
	}
	case VK_HOME:
		m_currentIndex = m_inputBuffer.rfind(L'\n', m_currentIndex);

		if (m_currentIndex == std::wstring::npos)
		{
			m_currentIndex = 0;
		}
		else ++m_currentIndex;

		break;
	case VK_END:
		m_currentIndex = m_inputBuffer.find(L'\n', m_currentIndex);

		if (m_currentIndex == std::wstring::npos)
		{
			m_currentIndex = m_inputBuffer.size() - 1;
		}
		else if (m_currentIndex > 0) --m_currentIndex;

		break;
	default:
		break;
	}
}


void ConsoleTextEditor::m_updateScreenBuffer() noexcept
{
	IndexType i = 0;
	IndexType t = 0;

	IndexType currColumnCount = 0;

	const auto consoleStartIndex = m_getConsoleStartIndex();
	const auto columnStartVal    = m_getConsoleColumnStartIndex(consoleStartIndex);		

	for (auto index = consoleStartIndex; index < m_inputBuffer.size(); ++index)
	{
		if (index == m_currentIndex)
		{
			m_setCursorPos(static_cast<short>(s_getMin(t, m_horizontalScrollThreshold())), static_cast<short>(i));
		}

		const auto character = m_inputBuffer.at(index); 

		switch (character)
		{
		case L'\n':

			m_drawPixel(t, i, true, false, false, false);

			if (++i >= m_screenHeight()) return;

			t = 0;
			currColumnCount = 0;
			
			break;
		case L'\t':
		{

			for (IndexType j = 0; j < s_tabSize; ++j)
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
				auto color = s_foregroundWhite;

				if (m_selectionInProgress)
				{
					const auto [min, max] = s_getMinMax(m_currentIndex, m_selectionStartIndex);

					if (min <= index && index <= max)
					{
						color = s_backgroundWhite;
					}

				}
		
				m_setGrid(t, i, character, color);
				
				++t;
			}
			break;
		}
	}
}

void ConsoleTextEditor::m_deleteCharAt(const IndexType index) noexcept
{
	const auto it = m_inputBuffer.begin() + index;

	m_handleCharDeletion(*it);
					
	m_inputBuffer.erase(it);
}

bool ConsoleTextEditor::m_deleteIfSelected() noexcept
{
	if (!m_selectionInProgress) return false;

	auto [min, max] = s_getMinMax(m_currentIndex, m_selectionStartIndex);

	if (max < m_inputBuffer.size() - 1) ++max;

	const auto startIt = m_inputBuffer.cbegin() + min;
	const auto endIt   = m_inputBuffer.cbegin() + max;

	for (auto it = startIt; it != endIt; ++it)
	{
		m_handleCharDeletion(*it);
	}

	m_inputBuffer.erase(startIt, endIt);

	m_selectionInProgress = false;
	m_currentIndex = min;

	return true;
}

void ConsoleTextEditor::m_scrollDownIfNeeded() noexcept
{
	const auto rowCount = m_getRowCountUntil(m_currentIndex);

	if (rowCount > m_verticalScrollThreshold())
	{
		m_startRow = rowCount - m_verticalScrollThreshold();
	}
}

void ConsoleTextEditor::m_handleCharDeletion(const wchar_t c) noexcept
{
	if (c == L'\n')
	{
		--m_rowCount;

		if (m_startRow > 0) --m_startRow;
	}
}

void ConsoleTextEditor::m_insertChar(const wchar_t c) noexcept
{
	m_inputBuffer.insert(m_inputBuffer.begin() + m_currentIndex, c);

	++m_currentIndex;
}

void ConsoleTextEditor::m_moveCursorOneWordLeft() noexcept
{
	const auto it = std::find_if(m_inputBuffer.crend() - m_currentIndex, m_inputBuffer.crend(),
	[&, findedWord = false] (const wchar_t element) mutable
	{
		
		if (std::iswalnum(element))
		{
			findedWord = true;
		}
		else
		{
			if (findedWord) return true;

			if (element == L'\n' && m_startRow > 0)
			{
				--m_startRow;
			}	
		}
		
		return false;
	});

	if (it != m_inputBuffer.crend())
	{
		m_currentIndex = std::distance(m_inputBuffer.cbegin(), it.base());
	}
	else m_currentIndex = 0;

}
void ConsoleTextEditor::m_moveCursorOneWordRight() noexcept
{
	auto it = std::find_if(m_inputBuffer.cbegin() + m_currentIndex + 1, m_inputBuffer.cend(),
	[&, findedWord = false] (const wchar_t element) mutable
	{
		if (std::iswalnum(element))
		{
			findedWord = true;
		}
		else if (findedWord) return true;
		
		return false;
	});

	if (it != m_inputBuffer.cend())
	{
		m_currentIndex = std::distance(m_inputBuffer.cbegin(), it) - 1;
	}
	else m_currentIndex = m_inputBuffer.size() - 1;

	m_scrollDownIfNeeded();
}

bool ConsoleTextEditor::m_readFile(const std::wstring_view filePath) noexcept
{
	std::FILE* file = nullptr;
	if (_wfopen_s(&file, filePath.data(), L"rb") || !file) return false;

	std::fgetwc(file); // read extra one space?

	m_inputBuffer.clear();
	
	m_selectionInProgress = false;
	m_startRow = 0;
	m_currentIndex = 0;
	m_rowCount = 0;

	std::wint_t c;
	
	while ((c = std::fgetwc(file)) != WEOF)
	{
		switch (c)
		{
		case L'\n':
			++m_rowCount;
		case L'\t':
			m_inputBuffer.push_back(c);
			break;
		default:

			if (std::iswprint(c))
			{
				m_inputBuffer.push_back(c);
			}
			break;
		}
	}

	std::fclose(file);

	return true;
}

bool ConsoleTextEditor::m_writeFile(const std::wstring_view filePath) const noexcept
{
	std::FILE* file = nullptr;
	if (_wfopen_s(&file, filePath.data(), L"wb") || !file) return false;

	std::fputws(m_inputBuffer.c_str(), file);

	std::fclose(file);

	return true;
}


[[nodiscard]] constexpr ConsoleTextEditor::IndexType ConsoleTextEditor::m_getConsoleStartIndex() const noexcept
{
	IndexType result = 0;

	if (m_startRow > 0)
	{
		IndexType currentRow = 0;
		
		for (const auto& element : m_inputBuffer)
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

[[nodiscard]] constexpr ConsoleTextEditor::IndexType ConsoleTextEditor::m_getConsoleColumnStartIndex(const IndexType consoleStartIndex) const noexcept
{
	IndexType result = 0;

	for (auto i = consoleStartIndex; i < m_currentIndex; ++i)
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

	if (result >= m_horizontalScrollThreshold()) return result - m_horizontalScrollThreshold() + 1;

	return 0;
}

[[nodiscard]] constexpr ConsoleTextEditor::IndexType ConsoleTextEditor::m_getRowCountUntil(const IndexType index) const noexcept
{
	IndexType result = 1;

	for (IndexType i = 0; i <= index; ++i)
	{
		if (m_inputBuffer.at(i) == L'\n') ++result;
	}

	return result;
}


void ConsoleTextEditor::m_updateConsole() noexcept
{
	m_clearConsole();

	m_updateScreenBuffer();
	
	m_renderConsole();
}

bool ConsoleTextEditor::s_setUserClipboard(const std::wstring_view str) noexcept
{
	if (!OpenClipboard(nullptr)) return false;

	const auto size = (str.size() + 1) * sizeof(wchar_t);

	const auto stringHandle = GlobalAlloc(GMEM_MOVEABLE, size);

	if (!stringHandle) return false;

	const auto lockedStr = GlobalLock(stringHandle);

	if (!lockedStr) return false;

	std::memcpy(lockedStr, str.data(), size);

	GlobalUnlock(stringHandle);

	if (!SetClipboardData(CF_UNICODETEXT, stringHandle)) return false;

	CloseClipboard();

	return true;
}

[[nodiscard]] std::optional<std::wstring> ConsoleTextEditor::s_getUserClipboard() noexcept
{	
	if (!OpenClipboard(nullptr)) return {};

	const auto clipboardHandle = GetClipboardData(CF_UNICODETEXT);
	
	if (!clipboardHandle) return {};
	
	const auto data = static_cast<wchar_t*>(GlobalLock(clipboardHandle));
	
	if (!data) return {};

	std::optional<std::wstring> result = { data };

	GlobalUnlock(clipboardHandle);

	CloseClipboard();

	return result;
}

