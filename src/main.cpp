#include "../include/console_text_editor.h"


int wmain(const int argc, const wchar_t* argv[])
{
	ConsoleTextEditor editor;

	if (!editor.m_constructEditor(argc, argv)) return -1;

	editor.m_run();
}
