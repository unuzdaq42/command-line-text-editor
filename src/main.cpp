#include "console_text_editor.h"


int main(const int argc, const char* argv[])
{
	ConsoleTextEditor editor;

	if (!editor.m_constructEditor(argc, argv)) return -1;

	editor.m_run();
}