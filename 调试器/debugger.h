#pragma once
#include<windows.h>

class debugger
{
public:
	BOOL isSystemPoint = TRUE;
	DEBUG_EVENT  debugEvent{};
	PROCESS_INFORMATION pi;
	void open(LPCSTR szPath);
	void run();
	LPVOID oep;
	DWORD dwContinueStatus = DBG_CONTINUE;
	HANDLE hProcess;
	HANDLE hThread;
	void openHandles();
	void closeHandles();
	void OnDispatchException();
	void GetInput();
};