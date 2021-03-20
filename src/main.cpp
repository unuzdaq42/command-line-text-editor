#include "console_text_editor.h"


int main(const int argc, const char* argv[])
{
	ConsoleTextEditor editor;

	if (!editor.m_constructEditor(argc, argv)) return -1;

	editor.m_run();
}


// #include "console.h"


// class ConsoleDeal : public Console
// {


// 	bool m_childConstruct() override { return true; }
	
// 	void m_childUpdate(const float) override 
// 	{
// 		m_clearConsole();
		
// 		for (auto i = 0; i < m_screenWidth() * m_screenHeight(); ++i)
// 		{
// 			m_setGrid(i, L'X');
// 		}
// 		m_consoleTitle = std::to_wstring(m_screenHeight());

// 		m_renderConsole();
// 	}


// 	void m_childHandleKeyEvents  (const KEY_EVENT_RECORD event) override
// 	{
// 		if (event.bKeyDown && event.wVirtualKeyCode == VK_ESCAPE) m_closeConsole();	

// 		if (event.bKeyDown && event.wVirtualKeyCode == VK_SPACE) m_resizeBuffer(m_screenWidth(), m_screenHeight() * 2);
// 	}

// };


// int main()
// {
// 	ConsoleDeal deal;

// 	if (!deal.m_construct(100, 100, 8, 8)) return -1;

// 	deal.m_run();

// }