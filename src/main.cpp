//#include <iostream>
//
//#include "olcConsoleGameEngine.h"
//
//
//class some : public olcConsoleGameEngine
//{
//public:
//
//	bool OnUserCreate() override
//	{
//		/*for (int i = 0; i < m_nScreenWidth * m_nScreenHeight; ++i)
//		{
//			m_bufScreen[i].Char.UnicodeChar = L'L';
//			m_bufScreen[i].Attributes = 0x000F;
//		}*/
//
//		return true;
//	}
//
//	bool OnUserUpdate(const float) override
//	{
//		this->DrawString(0, 0, L"selamlar t�rkiye");
//
//		
//		static bool printed = false;
//
//		if (printed)
//		{
//			return false;
//		}
//		else
//		{
//			system("pause");
//			return printed = true;
//		}
//
//	}
//
//
//};
//
//
//
//int main()
//{
//	some thing;
//	if (!thing.ConstructConsole(96, 96, 8, 8)) return -1;
//
//	thing.Start();
//
//	return 0;
//}


//#include <windows.h>
//#include <iostream>
//#include <stdio.h>
//
//constexpr short width = 80;
//constexpr short height = 2;
//
//int main()
//{
//    std::wcout << L"old buffer thingy\n";
//
//    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
//    
//    HANDLE hNewScreenBuffer = CreateConsoleScreenBuffer(
//        GENERIC_READ | GENERIC_WRITE,
//        FILE_SHARE_READ | FILE_SHARE_WRITE,        // shared
//        nullptr,                    // default security attributes
//        CONSOLE_TEXTMODE_BUFFER, // must be TEXTMODE
//        nullptr);               
//
//    if (hStdout == INVALID_HANDLE_VALUE || hNewScreenBuffer == INVALID_HANDLE_VALUE)
//    {
//        printf("CreateConsoleScreenBuffer failed - (%d)\n", GetLastError());
//        return 1;
//    }
//
//
//    if (!SetConsoleActiveScreenBuffer(hNewScreenBuffer))
//    {
//        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
//        return 1;
//    }
//
//    CHAR_INFO chiBuffer[width * height];
//
//    for (auto& elem : chiBuffer)
//    {
//        elem.Char.UnicodeChar = L'X';
//        elem.Attributes = 0xF;
//    }
//
//
//    SMALL_RECT srctWriteRect = { 0, 0, width - 1, height - 1 };
//
//    if(!WriteConsoleOutput(
//        hNewScreenBuffer, // screen buffer to write to
//        chiBuffer,        // buffer to copy from
//        { width, height },     // col-row size of chiBuffer
//        { 0, 0 },    // top left src cell in chiBuffer
//        &srctWriteRect))  // dest. screen buffer rectangle
//    {
//        printf("WriteConsoleOutput failed - (%d)\n", GetLastError());
//        return 1;
//    }
//    
//    std::wcin.get();
//
//    // Restore the original active screen buffer.
//
//    if (!SetConsoleActiveScreenBuffer(hStdout))
//    {
//        printf("SetConsoleActiveScreenBuffer failed - (%d)\n", GetLastError());
//        return 1;
//    }
//
//    return 0;
//}

// main.cpp

#include "console.h"

// TODO

// convert console to old state after done with it

// default max buffer constuctor

// make default cursor workable
// its just sits there 

int main()
{
	console console(20, 20, 16, 16);

	if (!console.m_construct()) return -1;
	console.m_run();

	// std::cin.get();

	return 0;
}