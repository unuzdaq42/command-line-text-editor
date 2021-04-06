#include "../include/console_text_editor.h"

#include <sstream>
#include <cwctype> // std::iswprint
#include <algorithm>
#include <cwchar>

void TextEditor::m_initEditor(const SizeType width, const SizeType height,
	const WORD textColor, const SizeType startX, const SizeType startY)
{
	m_width  = width;
	m_height = height;

	m_drawStartX = startX;
	m_drawStartY = startY;

	m_textColor = textColor;

	if (m_inputBuffer.empty()) m_inputBuffer.push_back(L' ');
}

bool TextEditor::m_handleEvents(const Console& console, const KEY_EVENT_RECORD& event)
{
	if (m_lastEvent == EventType::MouseWheel)
	{
		console.m_setCursorInfo(true);
	}

	m_lastEvent = EventType::Keyboard;

	if (event.bKeyDown && event.wVirtualKeyCode == VK_ESCAPE)
	{
		if (m_selectionInProgress) 
		{ 
			// cancel selection
			m_selectionInProgress = false; 
			return true;
		}
		else return false; // close editor
	}

	m_handleInsertionEvents (event);
	m_handleCursorEvents    (event);
	m_handleControlKeyEvents(event);

	return true;
}

void TextEditor::m_handleEvents(const Console& console, const MOUSE_EVENT_RECORD& event)
{
	const auto state = console.m_leftMouseButton.m_state();

	switch (state)
	{
	case Console::ButtonState::Pressed:
	case Console::ButtonState::Held:
	{
		const auto mouseIndex = m_getIndexAtPos(event.dwMousePosition.X, event.dwMousePosition.Y);

		if 		(event.dwMousePosition.Y <= m_drawStartY + 1) m_scrollOneUp();
		else if (event.dwMousePosition.Y >= m_drawStartY + m_height - 2) m_scrollOneDown();

		if (state == Console::ButtonState::Held)
		{	
			m_handleSelection(mouseIndex, m_currentIndex);
		}
		else
		{	
			m_selectionInProgress = false;
			m_currentIndex = mouseIndex;
		}

		if (m_lastEvent == EventType::MouseWheel)
		{
			console.m_setCursorInfo(true);
		}

		m_lastEvent = EventType::MouseButton;

		break;
	}
	case Console::ButtonState::Released:
		break;
	}
	
	if (event.dwEventFlags == MOUSE_WHEELED)
	{
		const short wheelRotation = HIWORD(event.dwButtonState);

		if (wheelRotation < 0) m_scrollOneDown();
		else m_scrollOneUp();

		m_lastEvent = EventType::MouseWheel;

		console.m_setCursorInfo(false);
	}
}


void TextEditor::m_syncHeightWithRows(const SizeType consoleHeight) noexcept
{
	if (m_rowCount <= 5)
	{
		m_height = m_rowCount;
		m_drawStartY = consoleHeight - m_height;
	}
}


void TextEditor::m_handleInsertionEvents(const KEY_EVENT_RECORD& event)
{	
	if (!event.bKeyDown || Console::s_isCtrlKeyPressed(event)) return;

	switch (event.wVirtualKeyCode)
	{
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

		break;
	default:

		if (event.uChar.UnicodeChar)
		{
			m_deleteIfSelected();

			m_insertChar(event.uChar.UnicodeChar);
		}

		break;    
	}

	return;
}

void TextEditor::m_handleCursorEvents(const KEY_EVENT_RECORD& event)
{
	if (Console::s_isShiftKeyEvent(event))
	{
		m_shiftPressed = event.bKeyDown;
	}

	const auto oldInputIndex = m_currentIndex;
	
	m_handleCursorMoveEvents(event);

	if (oldInputIndex != m_currentIndex)
	{
		// user moved the cursor

		if (m_shiftPressed && !m_selectionInProgress)
		{
			m_handleSelection(oldInputIndex);
		}

		m_selectionInProgress = m_shiftPressed;
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
	
		break;
	}
	case VirtualKeyCode::Z:
	{
		// undo event
		if (m_records.empty()) break;

		std::visit(utils::MakeVisitor
		{ 
			[&] (const InsertionRecord& record)
			{
				m_deleteStartingFrom(record.m_index, record.m_index + record.m_size);
			},
			[&] (const DeletionRecord& record)
			{
				m_insertString(record.m_data, record.m_index);
			}
		}, m_records.back());

		m_records.pop_back();
		
		break;
	}
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
	case VK_DOWN:
		
		m_moveCursorOneLineDown();
		m_scrollOneDown();
		break;
	case VK_UP:

		m_moveCursorOneLineUp();
		m_scrollOneUp();
		break;
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

		if (m_currentIndex > 0) --m_currentIndex;

		break;
	case VK_RIGHT:

		if (m_currentIndex + 1 < m_inputBuffer.size()) ++m_currentIndex;

		break;
	case VK_UP:
		
		m_moveCursorOneLineUp();
		break;
	case VK_DOWN:
		m_moveCursorOneLineDown();
		break;
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
	if (m_lastEvent == EventType::Keyboard) { m_updateStartRow(); }

	SizeType i = 0;
	SizeType t = 0;

	SizeType currColumnCount = 0;

	const auto consoleStartIndex = m_getConsoleStartIndex();
	const auto columnStartVal    = m_getConsoleColumnStartIndex(consoleStartIndex);

	const auto searchStrSize = searchStr.size();
	SizeType searchIndex = std::wstring::npos;

	for (auto index = consoleStartIndex; index < m_inputBuffer.size(); ++index)
	{
		if (index == m_currentIndex)
		{
			m_cursorPos = { static_cast<short>(m_drawStartX + std::min(t, m_width / 2)), static_cast<short>(m_drawStartY + i) };
		}

		if (searchStrSize > 0 && index + searchStrSize < m_inputBuffer.size())
		{
			if (searchStr == std::wstring_view{ m_inputBuffer.c_str() + index, searchStrSize })
			{
				searchIndex = index;
			}
		}

		const auto consoleIndex = console.m_getIndex(m_drawStartX + t, m_drawStartY + i);
		const auto character = m_inputBuffer.at(index); 

		switch (character)
		{
		case L'\n':
	
			if (++i >= m_height) return;
			
			t = 0;
			currColumnCount = 0;
			
			break;
		case L'\t':
		
			if ((currColumnCount += s_tabSize) <= columnStartVal) break;

			if (columnStartVal + s_tabSize < currColumnCount) t += s_tabSize;    
			else t += currColumnCount - columnStartVal;
			
			break;
		default:
		
			if (t < m_width && ++currColumnCount > columnStartVal)
			{
				auto color = m_textColor;

				if (m_selectionInProgress)
				{
					const auto [min, max] = utils::GetMinMax(m_currentIndex, m_selectionStartIndex);

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
			WORD color;

			if (searchIndex <= m_currentIndex && m_currentIndex <= searchIndex + searchStrSize)
			{
				color = BACKGROUND_GREEN | BACKGROUND_BLUE;
			}
			else color = BACKGROUND_RED | BACKGROUND_GREEN;

			console.m_setColorAt(consoleIndex, console.m_getColorAt(consoleIndex) | color);
		}

	}
}


void TextEditor::m_deleteCharAt(const SizeType index) noexcept
{
	const auto it = m_inputBuffer.begin() + index;

	m_writeDeletionRecord(m_currentIndex, { m_inputBuffer.at(index) }, std::iswcntrl(*it));

	if (*it == L'\n') --m_rowCount;
			
	m_inputBuffer.erase(it);
}

bool TextEditor::m_deleteIfSelected() noexcept
{
	if (!m_selectionInProgress) return false;

	const auto [min, max] = utils::GetMinMax(m_currentIndex, m_selectionStartIndex);

	m_writeDeletionRecord(min, m_inputBuffer.substr(min, max - min + 1));

	m_deleteStartingFrom(min, max + 1);

	m_selectionInProgress = false;

	return true;
}

void TextEditor::m_deleteStartingFrom(const SizeType start, SizeType end) noexcept
{	
	if (end >= m_inputBuffer.size()) end = m_inputBuffer.size() - 1;

	const auto startIt = m_inputBuffer.cbegin() + start;
	const auto endIt   = m_inputBuffer.cbegin() + end;

	for (auto it = startIt; it != endIt; ++it)
	{
		if (*it == L'\n') --m_rowCount;
	}

	m_inputBuffer.erase(startIt, endIt);
	m_currentIndex = start;
}

void TextEditor::m_insertChar(const wchar_t c) noexcept
{
	m_writeInsertionRecord(m_currentIndex, 1, std::iswcntrl(c));

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
	SizeType index;
	
	if (m_selectionInProgress)
	{
		index = std::min(m_currentIndex, m_selectionStartIndex);
	}
	else index = m_currentIndex;

	m_insertString(str, index);

	m_writeInsertionRecord(index, str.size());
}

void TextEditor::m_insertString(const std::wstring_view str, const SizeType insertIndex)
{	
	m_deleteIfSelected();
	
	m_rowCount += std::count(m_inputBuffer.cbegin(), m_inputBuffer.cend(), L'\n');

	const auto secondPartSize = m_inputBuffer.size() - insertIndex;

	m_inputBuffer.resize(m_inputBuffer.size() + str.size());
	
	for (SizeType i = 0; i < secondPartSize; ++i)
	{
		const auto index = insertIndex + (secondPartSize - i - 1);

		m_inputBuffer.at(index + str.size()) = m_inputBuffer.at(index);
	}
	
	for (SizeType i = 0; i < str.size(); ++i)
	{
		m_inputBuffer.at(i + insertIndex) = str.at(i);
	}

	m_currentIndex = insertIndex + str.size();
}

void TextEditor::m_replaceMatchsWith(const std::wstring_view keyStr, const std::wstring_view replaceStr) noexcept
{
	m_currentIndex = 0;

	while (m_selectNextString(keyStr))
	{
		m_insertString(replaceStr);
	}
}

namespace
{

	[[nodiscard]] constexpr auto GetWordMoveFunctor() noexcept
	{
		return [findedWord = false] (const wchar_t element) mutable
		{
			if (std::iswalnum(element))
			{
				findedWord = true;
			}
			else if (findedWord) return true;
			
			return false;
		};
	}

}

void TextEditor::m_moveCursorOneWordLeft() noexcept
{
	const auto it = std::find_if(m_inputBuffer.crend() - m_currentIndex, m_inputBuffer.crend(), GetWordMoveFunctor());

	if (it != m_inputBuffer.crend())
	{
		m_currentIndex = std::distance(m_inputBuffer.cbegin(), it.base());
	}
	else m_currentIndex = 0;

}
void TextEditor::m_moveCursorOneWordRight() noexcept
{
	const auto it = std::find_if(m_inputBuffer.cbegin() + m_currentIndex + 1, m_inputBuffer.cend(), GetWordMoveFunctor());

	if (it != m_inputBuffer.cend())
	{
		m_currentIndex = std::distance(m_inputBuffer.cbegin(), it) - 1;
	}
	else m_currentIndex = m_inputBuffer.size() - 1;
}

void TextEditor::m_moveCursorOneLineDown() noexcept
{
	SizeType colCount = 0;

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

	const auto size = std::min(newLineIndex + colCount, m_inputBuffer.size() - 1);

	auto i = newLineIndex;

	for (; i < size; ++i)
	{
		if (m_inputBuffer.at(i) == L'\n') break;
	}

	m_currentIndex = i;
}

void TextEditor::m_moveCursorOneLineUp() noexcept
{
	if (m_currentIndex == 0) return;

	const auto firstNewLineIndex = m_inputBuffer.rfind(L'\n', m_currentIndex - 1);

	if (firstNewLineIndex == 0) { m_currentIndex = 0; return; }
	if (firstNewLineIndex == std::wstring::npos) return;

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

}



bool TextEditor::m_readFile(const std::wstring_view filePath) noexcept
{
	std::FILE* file = nullptr;
	if (_wfopen_s(&file, filePath.data(), L"r, ccs=UTF-8") || !file) return false;

	m_inputBuffer.clear();
	
	m_selectionInProgress = false;
	m_currentIndex = 0;
	m_rowCount = 1;

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
	if (m_inputBuffer.size() <= 2) return false;

	std::FILE* file = nullptr;
	if (_wfopen_s(&file, filePath.data(), L"w+, ccs=UTF-8") || !file) return false;
	
	// _wfopen_s adds BOM to start of the file
	// could not find a way to close it
	std::rewind(file); // dirty but works

	std::fputws(m_inputBuffer.c_str(), file);

	std::fclose(file);

	return true;
}

void TextEditor::m_writeInsertionRecord(const SizeType index, const SizeType size, const bool createNew) noexcept
{
	if (!m_records.empty() && !createNew)
	{
		auto& last = m_records.back();

		if (std::holds_alternative<InsertionRecord>(last))
		{
			auto& value = std::get<InsertionRecord>(last);

			if (value.m_index + value.m_size == index)
			{
				++value.m_size;
				return;
			}
		}
		
	}

	m_records.emplace_back(InsertionRecord{ index, size });

	m_resizeRecordsIfNeeded();
}

void TextEditor::m_writeDeletionRecord(const SizeType index, std::wstring&& str, const bool createNew) noexcept
{	
	if (!m_records.empty() && !createNew && str.size() == 1)
	{
		auto& last = m_records.back();

		if (std::holds_alternative<DeletionRecord>(last))
		{
			auto& value = std::get<DeletionRecord>(last);

			if (value.m_index == index)
			{
				value.m_data += str;
				return;
			}

			if (index + 1 == value.m_index)
			{
				value.m_data = str + value.m_data;
				value.m_index = index;
				return;
			}
		}
	}

	m_records.emplace_back(DeletionRecord{ index, std::forward<std::wstring>(str) });

	m_resizeRecordsIfNeeded();
}

void TextEditor::m_resizeRecordsIfNeeded() noexcept
{
	constexpr SizeType maxLimit = 100;

	if (m_records.size() > maxLimit)
	{
		m_records.erase(m_records.begin(), m_records.begin() + m_records.size() - maxLimit);
	}
}


[[nodiscard]] TextEditor::SizeType TextEditor::m_getIndexAtPos(const SizeType x, const SizeType y) const noexcept
{
	const auto startIndex = m_getConsoleStartIndex();

	auto i = startIndex;

	auto currentX = m_drawStartX;
	auto currentY = m_drawStartY;

	for (; i < m_inputBuffer.size() - 1; ++i)
	{
		if (currentX == x && currentY == y) break;

		switch (m_inputBuffer.at(i))
		{
		case L'\n':
			currentX = 0;
			if (++currentY > y) return i;
			break;
		case L'\t':
			currentX += s_tabSize;
			break;
		default:
			++currentX;
			break;
		}
	}

	return i;
}

constexpr void TextEditor::m_updateStartRow() noexcept
{
	const auto rowCount = m_getRowCountUntil(m_currentIndex);

	if (rowCount > m_startRow + m_height)
	{
		m_startRow = rowCount - m_height;
	}
	else if (rowCount <= m_startRow)
	{
		m_startRow = rowCount - 1;
	}
}

[[nodiscard]] constexpr TextEditor::SizeType TextEditor::m_getConsoleStartIndex() const noexcept
{
	SizeType result = 0;

	if (m_startRow > 0)
	{
		SizeType currentRow = 0;
		
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

[[nodiscard]] constexpr TextEditor::SizeType TextEditor::m_getConsoleColumnStartIndex(const SizeType consoleStartIndex) const noexcept
{
	if (m_lastEvent != EventType::Keyboard) return 0;

	SizeType result = 0;

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

	if (result >= m_width / 2) return result - m_width / 2 + 1;

	return 0;
}

[[nodiscard]] constexpr TextEditor::SizeType TextEditor::m_getRowCountUntil(const SizeType index) const noexcept
{
	SizeType result = 1;

	for (SizeType i = 0; i < index; ++i)
	{
		if (m_inputBuffer.at(i) == L'\n') ++result;
	}

	return result;
}

constexpr void TextEditor::m_handleSelection(const SizeType start) noexcept
{
	m_selectionInProgress = true;
	m_selectionStartIndex = start;

	const auto diff = utils::GetSignedDiff(m_currentIndex, start);

	if (diff == 1 || diff == -1)
	{
		m_currentIndex -= diff;
	}
}

constexpr void TextEditor::m_handleSelection(const SizeType start, const SizeType end) noexcept
{   
	m_selectionInProgress = true;
	m_selectionStartIndex = start;
	m_currentIndex = std::min(end, m_inputBuffer.size() - 1);
}

bool TextEditor::m_selectNextString(const std::wstring_view str) noexcept
{
	if (str.empty() || str.size() > m_inputBuffer.size()) return false;

	auto i = m_currentIndex;
	
	if (str.size() == 1) ++i;

	for (; str != std::wstring_view{ m_inputBuffer.c_str() + i, str.size() }; ++i)
	{
		if (i >= m_inputBuffer.size() - str.size()) return false;
	}

	m_handleSelection(i, i + str.size() - 1);

	return true;
}

bool TextEditor::m_selectPreviousString(const std::wstring_view str) noexcept
{
	if (str.empty() || str.size() > m_inputBuffer.size() || m_currentIndex < str.size()) return false;

	auto i = m_currentIndex - str.size();

	for (; str != std::wstring_view{ m_inputBuffer.c_str() + i, str.size() }; --i)
	{
		if (i == 0) return false;
	}

	m_handleSelection(i, i + str.size() - 1);

	return true;
}

[[nodiscard]] std::pair<TextEditor::SizeType, TextEditor::SizeType> 
TextEditor::m_getMatchResults(const std::wstring_view str) const noexcept
{
	SizeType beforeInd   = 0;
	SizeType totalResult = 0;

	if (!str.empty() && str.size() < m_inputBuffer.size())
	{	
		for (SizeType i = 0; i < m_inputBuffer.size() - str.size(); ++i)
		{
			std::wstring_view currStr = { m_inputBuffer.c_str() + i, str.size() };

			if (currStr == str)
			{
				if (i + str.size() < m_currentIndex) ++beforeInd;

				++totalResult;
			}
		}
	}

	if (totalResult > 0 && beforeInd < totalResult) ++beforeInd;

	return { beforeInd, totalResult };
}


void TextEditor::m_setInputBuffer(const std::wstring_view str) noexcept
{
	m_inputBuffer.clear();
	m_inputBuffer.reserve(str.size() + 1);

	m_rowCount = 1;

	for (const auto element : str)
	{
		if (element == L'\n') ++m_rowCount;

		m_inputBuffer.push_back(element);
	}

	m_inputBuffer.push_back(L' ');

	m_currentIndex = m_inputBuffer.size() - 1;
	m_selectionInProgress = false;  
}