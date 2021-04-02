#include "console_text_editor.h"

#include <sstream>
#include <cwctype> // std::iswprint
#include <algorithm>
#include <cwchar>


void TextEditor::m_initEditor(const IndexType width, const IndexType height,
	const WORD textColor, const IndexType startX, const IndexType startY)
{
	m_width  = width;
	m_height = height;

	m_drawStartX = startX;
	m_drawStartY = startY;

	m_textColor = textColor;

	m_verScrollThreshold = height / 2;
	m_horScrollThreshold = width / 2;

	if (m_inputBuffer.empty()) m_inputBuffer.push_back(L' ');
}

bool TextEditor::m_handleEvents(const KEY_EVENT_RECORD& event)
{
	if (!m_handleKeyEvents(event)) return false;

	m_handleCursorEvents(event);
	m_handleControlKeyEvents(event);

	return true;
}

void TextEditor::m_syncHeightWithRows(const IndexType consoleHeight) noexcept
{
	if (m_rowCount <= 5)
	{
		m_height = m_rowCount;
		m_drawStartY = consoleHeight - m_height;
		m_verScrollThreshold = std::wstring::npos;
	}
	else
	{
		m_verScrollThreshold = m_height;
		m_scrollDownIfNeeded();
	}
}


bool TextEditor::m_handleKeyEvents(const KEY_EVENT_RECORD& event)
{	
	if (!event.bKeyDown || Console::s_isCtrlKeyPressed(event)) return true;

	switch (event.wVirtualKeyCode)
	{
	case VK_ESCAPE:

		if (m_selectionInProgress) 
		{ 
			// cancel selection
			m_selectionInProgress = false; 
			m_scrollDownIfNeeded(); 
		}
		else return false; // close editor

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

	return true;
}

void TextEditor::m_handleCursorEvents(const KEY_EVENT_RECORD& event)
{
	static bool s_shiftPressed = false;

	if (Console::s_isShiftKeyEvent(event))
	{
		s_shiftPressed = event.bKeyDown;
	}

	const auto oldInputIndex = m_currentIndex;
	
	m_handleCursorMoveEvents(event);

	if (oldInputIndex != m_currentIndex)
	{
		// user moved the cursor

		if (s_shiftPressed && !m_selectionInProgress)
		{
			m_handleSelection(oldInputIndex);
		}

		m_selectionInProgress = s_shiftPressed;
	}
}

void TextEditor::m_handleControlKeyEvents(const KEY_EVENT_RECORD& event)
{
	if (!event.bKeyDown || !Console::s_isCtrlKeyPressed(event)) return;

	switch (event.wVirtualKeyCode)
	{
	case VirtualKeyCode::C:
	case VirtualKeyCode::X:
	{
		// Copy and Cut event

		const auto str = m_getSelectedStr();

		if (str.has_value())
		{			
			// check errors?
			Console::s_setUserClipboard(str.value());

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
		auto clipboard = Console::s_getUserClipboard();

		if (clipboard.has_value()) m_insertUnsafeString(std::move(clipboard.value()));
	
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

		const auto start = m_inputBuffer.rfind(L'\n', findStart);
		const auto end = m_inputBuffer.find(L'\n', m_currentIndex);

		if (start == std::wstring::npos) m_currentIndex = 0;
		else m_currentIndex = start + 1;

		m_selectionStartIndex = std::min(end, m_inputBuffer.size() - 1);

		m_selectionInProgress = true;

		break;
	}
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


void TextEditor::m_handleCursorMoveEvents(const KEY_EVENT_RECORD& event)
{
	if (!event.bKeyDown) return;

	if (Console::s_isCtrlKeyPressed(event))
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

void TextEditor::m_updateConsole(Console& console, const std::wstring_view searchStr) noexcept
{
	console.m_drawRect(m_drawStartX, m_drawStartY, m_width, m_height, m_textColor);

	IndexType i = 0;
	IndexType t = 0;

	IndexType currColumnCount = 0;

	const auto consoleStartIndex = m_getConsoleStartIndex();
	const auto columnStartVal    = m_getConsoleColumnStartIndex(consoleStartIndex);

	const auto searchStrSize = searchStr.size();
	IndexType searchIndex = std::wstring::npos;

	for (auto index = consoleStartIndex; index < m_inputBuffer.size(); ++index)
	{
		if (index == m_currentIndex)
		{
			m_cursorPos = { static_cast<short>(m_drawStartX + s_getMin(t, m_horScrollThreshold)), static_cast<short>(m_drawStartY + i) };
		}

		if (searchStrSize > 0 && index + searchStrSize < m_inputBuffer.size())
		{
			std::wstring_view current{ m_inputBuffer.c_str() + index, searchStrSize };

			if (current == searchStr)
			{
				searchIndex = index;
			}
		}

		const auto consoleIndex = console.m_getIndex(m_drawStartX + t, m_drawStartY + i);
		const auto character = m_inputBuffer.at(index); 

		switch (character)
		{
		case L'\n':
	
			console.m_setGrid(consoleIndex, L'\\', m_textColor);

			if (++i >= m_height) { console.m_renderConsole(); return; }
			
			t = 0;
			currColumnCount = 0;
			
			break;
		case L'\t':
		{

			for (IndexType j = 0; j < s_tabSize; ++j)
			{
				console.m_drawPixel(m_drawStartX + t + j, m_drawStartY + i, false, true, false, false);
			}

			const auto oldCount = currColumnCount;

			if ((currColumnCount += s_tabSize) <= columnStartVal) break;

			if (oldCount > columnStartVal) t += s_tabSize;    
			else t += currColumnCount - columnStartVal;
		}
			break;
		default:
		
			if (t < m_width && ++currColumnCount > columnStartVal)
			{
				auto color = m_textColor;

				if (m_selectionInProgress)
				{
					const auto [min, max] = s_getMinMax(m_currentIndex, m_selectionStartIndex);

					if (min <= index && index <= max)
					{
						color |= Console::s_backgroundWhite;
					}

				}
		
				console.m_setGrid(consoleIndex, character, color);
				
				++t;
			}
			break;
		}

		if (searchIndex != std::wstring::npos && index - searchIndex < searchStrSize)
		{
			console.m_setColorAt(consoleIndex, console.m_getColorAt(consoleIndex) | BACKGROUND_RED | BACKGROUND_GREEN);
		}

	}
}

void TextEditor::m_handleCharDeletion(const wchar_t c) noexcept
{
	if (c == L'\n')
	{
		--m_rowCount;

		if (m_startRow > 0) --m_startRow;
	}
}

void TextEditor::m_deleteCharAt(const IndexType index) noexcept
{
	const auto it = m_inputBuffer.begin() + index;

	m_handleCharDeletion(*it);
					
	m_inputBuffer.erase(it);
}

bool TextEditor::m_deleteIfSelected() noexcept
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

void TextEditor::m_scrollDownIfNeeded() noexcept
{
	const auto rowCount = m_getRowCountUntil(m_currentIndex);

	if (rowCount > m_verScrollThreshold)
	{
		m_startRow = rowCount - m_verScrollThreshold;
	}
}

void TextEditor::m_insertChar(const wchar_t c) noexcept
{
	m_inputBuffer.insert(m_inputBuffer.begin() + m_currentIndex, c);

	++m_currentIndex;
}

void TextEditor::m_insertUnsafeString(std::wstring str)
{
	// preprocess string before putting it to input buffer
	for (auto it = str.cbegin(); it != str.cend();)
	{
		const auto element = *it;

		switch (element)
		{
		case L'\n':
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

	m_insertString(str);
}

void TextEditor::m_insertString(const std::wstring_view str)
{
	m_deleteIfSelected();

	for (const auto elem : str) { if (elem == L'\n') ++m_rowCount; }

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

void TextEditor::m_replaceMatchsWith(const std::wstring_view keyStr, const std::wstring_view replaceStr) noexcept
{
	m_currentIndex = 0;

	while (m_selectNextString(keyStr))
	{
		m_insertString(replaceStr);
	}
}


void TextEditor::m_moveCursorOneWordLeft() noexcept
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
void TextEditor::m_moveCursorOneWordRight() noexcept
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

bool TextEditor::m_readFile(const std::wstring_view filePath) noexcept
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
	
	m_inputBuffer.push_back(L' ');

	std::fclose(file);

	return true;
}

bool TextEditor::m_writeFile(const std::wstring_view filePath) const noexcept
{
	std::FILE* file = nullptr;
	if (_wfopen_s(&file, filePath.data(), L"wb") || !file) return false;

	std::fputws(m_inputBuffer.c_str(), file);

	std::fclose(file);

	return true;
}


[[nodiscard]] constexpr TextEditor::IndexType TextEditor::m_getConsoleStartIndex() const noexcept
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

[[nodiscard]] constexpr TextEditor::IndexType TextEditor::m_getConsoleColumnStartIndex(const IndexType consoleStartIndex) const noexcept
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

	if (result >= m_horScrollThreshold) return result - m_horScrollThreshold + 1;

	return 0;
}

[[nodiscard]] constexpr TextEditor::IndexType TextEditor::m_getRowCountUntil(const IndexType index) const noexcept
{
	IndexType result = 1;

	for (IndexType i = 0; i <= index; ++i)
	{
		if (m_inputBuffer.at(i) == L'\n') ++result;
	}

	return result;
}

[[nodiscard]] std::pair<std::size_t, std::size_t> TextEditor::m_getMatchResults(const std::wstring_view str) const noexcept
{
	std::size_t beforeInd   = 0;
	std::size_t totalResult = 0;

	if (!str.empty() && str.size() < m_inputBuffer.size())
	{	
		for (std::size_t i = 0; i < m_inputBuffer.size() - str.size(); ++i)
		{
			std::wstring_view currStr = { m_inputBuffer.c_str() + i, str.size() };

			if (currStr == str)
			{
				if (i + str.size() <= m_currentIndex) ++beforeInd;

				++totalResult;
			}
		}
	}

	if (beforeInd > 0) ++beforeInd;

	return { beforeInd, totalResult };
}

constexpr void TextEditor::m_handleSelection(const IndexType start) noexcept
{
	m_selectionInProgress = true;
	m_selectionStartIndex = start;

	const auto diff = s_getSignedDiff(m_currentIndex, start);

	if (diff == 1 || diff == -1)
	{
		m_currentIndex -= diff;
	}
}

constexpr void TextEditor::m_handleSelection(const IndexType start, const IndexType end) noexcept
{   
	m_selectionInProgress = true;
	m_selectionStartIndex = start;
	m_currentIndex = s_getMin(end, m_inputBuffer.size() - 1);
}

bool TextEditor::m_selectNextString(const std::wstring_view str) noexcept
{
	if (str.empty() || str.size() > m_inputBuffer.size()) return false;

	auto i = m_currentIndex + 1;

	std::wstring_view buffer;

	for (; str != (buffer = std::wstring_view{ m_inputBuffer.c_str() + i, str.size() }); ++i)
	{
		if (i >= m_inputBuffer.size() - str.size()) return false;
	}

	m_handleSelection(i, i + str.size() - 1);

	m_scrollDownIfNeeded();

	return true;
}

bool TextEditor::m_selectPreviousString(const std::wstring_view str) noexcept
{
	if (str.empty() || str.size() > m_inputBuffer.size() || m_currentIndex < str.size()) return false;

	auto i = m_currentIndex - str.size();
	IndexType newLineCounter = 0;

	for (; str != std::wstring_view{ m_inputBuffer.c_str() + i, str.size() }; --i)
	{
		if (i == 0) return false;

		if (m_inputBuffer.at(i) == L'\n') ++newLineCounter;
	}

	m_startRow = (m_startRow > newLineCounter) ? m_startRow - newLineCounter : 0;

	m_handleSelection(i, i + str.size() - 1);

	return true;
}



void TextEditor::m_setInputBuffer(const std::wstring_view str) noexcept
{
	m_inputBuffer.clear();
	m_inputBuffer.reserve(str.size() + 1);

	for (const auto element : str)
	{
		if (element == L'\n') ++m_rowCount;

		m_inputBuffer.push_back(element);
	}

	m_inputBuffer.push_back(L' ');

	m_currentIndex = m_inputBuffer.size() - 1;
	m_rowCount = 1;
	m_selectionInProgress = false;  

	m_scrollDownIfNeeded();
}