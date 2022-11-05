#include"debugger.h"
#include"capstone.h"
#include"breakpoint.h"
#include<DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")
extern LPVOID oep;
extern PreviousbpInfo bpInfo;
extern LPVOID addr;
extern int type;
int flag = 0;
int nIsGo = 0;
void debugger::open(LPCSTR szPath)
{
	STARTUPINFOA si{ sizeof(si) };

	BOOL bResult = CreateProcessA(szPath,
		NULL,
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS,
		NULL,
		NULL,
		&si,
		&pi
	);
	if (!bResult)
	{
		printf("���̴���ʧ��!");
	}

	Capstone::Init();
}

void debugger::openHandles()
{
	hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, debugEvent.dwProcessId);
	hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, debugEvent.dwThreadId);
}

void debugger::closeHandles()
{

	CloseHandle(hProcess);
	CloseHandle(hThread);
}

void debugger::run()
{

	while (WaitForDebugEvent(&debugEvent, -1))
	{
		openHandles();
		switch (debugEvent.dwDebugEventCode)
		{
		case EXCEPTION_DEBUG_EVENT:
			OnDispatchException();
			break;
		case CREATE_PROCESS_DEBUG_EVENT:
			SymInitialize(pi.hProcess, NULL, FALSE);
			SymLoadModule64(pi.hProcess, debugEvent.u.CreateProcessInfo.hFile, NULL, NULL, (DWORD64)debugEvent.u.CreateProcessInfo.lpBaseOfImage, 0);
			::oep = oep = debugEvent.u.CreateProcessInfo.lpStartAddress;
			break;
		case LOAD_DLL_DEBUG_EVENT:
			SymInitialize(pi.hProcess, NULL, FALSE);
			SymLoadModule64(pi.hProcess, debugEvent.u.LoadDll.hFile, NULL, NULL, (DWORD64)debugEvent.u.LoadDll.lpBaseOfDll, 0);
			//SymLoadModule64(pi.hProcess,debugEvent.u.LoadDll.hFile,GetModuleFileName())
			break;
		}

		ContinueDebugEvent(debugEvent.dwProcessId,
			debugEvent.dwThreadId,
			dwContinueStatus);
		closeHandles();
	}
}

void debugger::OnDispatchException()
{
	auto exceptionCode = debugEvent.u.Exception.ExceptionRecord.ExceptionCode;
	auto exceptionAddr = debugEvent.u.Exception.ExceptionRecord.ExceptionAddress;
	auto exceptionType = debugEvent.u.Exception.ExceptionRecord.ExceptionInformation[0];
	switch (exceptionCode)
	{
	case EXCEPTION_BREAKPOINT:
		if (isSystemPoint)
		{
			Breakpoint::SetInt3BreakPoint(hProcess, oep);
			isSystemPoint = FALSE;
		}
		dwContinueStatus = Breakpoint::FixInt3BreakPoint(hProcess, exceptionAddr, hThread);
		break;
	case EXCEPTION_SINGLE_STEP:
		//�޸��ϵ�����tf�ߵ���һ���������öϵ�
		if (bpInfo.previouseIsBp == 1)
		{
			if (bpInfo.whichbp == 0)
			{
				Breakpoint::SetInt3BreakPoint(hProcess, bpInfo.PreviousAddr);
			}
			else if (bpInfo.whichbp == 1)
			{
				Breakpoint::SetHwBreakPoint(hThread, bpInfo.PreviousAddr, bpInfo.type, bpInfo.len);
			}
			else if (bpInfo.whichbp == 2)
			{
				Breakpoint::SetMmBreakPoint(hProcess, addr,type);
			}
			//����ǰһ���Ƿ�����tf�ϵ�ϵ�������
			bpInfo.previouseIsBp = 0;
			//tf�ߵ���һ���������ã��˴�����ӡ���
			return;
		}
		//���������޸�Ӳ���ϵ��tf������������
		else if(bpInfo.previouseIsBp == 0)
		{
			//����tf��
			if (flag == 1)
			{
				flag = 0;
				goto x;
			}
			Breakpoint::FixHwBreakPoint(hThread, hProcess, exceptionAddr);
			//������������ӡ���
		}
		break;
	case EXCEPTION_ACCESS_VIOLATION:
		auto dwMemAddr = debugEvent.u.Exception.ExceptionRecord.ExceptionInformation[1];
		Breakpoint::FixMmBreakPoint(hProcess, hThread, (LPVOID)dwMemAddr, exceptionType);
		if (nIsGo == 0)
		{
			return;
		}
		else if (nIsGo == 1)
		{
			nIsGo = 0;
		}
		break;
	}

x:	Capstone::DisAsm(hProcess, exceptionAddr, 10);
	GetInput();
}

void getSymFromAddr(HANDLE hProcess, DWORD dwAddress)
{
	DWORD64  dwDisplacement = 0;

	char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
	PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

	pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSymbol->MaxNameLen = MAX_SYM_NAME;

	if (SymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol)) {
		printf("�ҵ���ַ[%lx]:%s\n", dwAddress, pSymbol->Name);
	}
	else {
		// SymFromAddr failed
		DWORD error = GetLastError();
		printf("SymFromAddr returned error : %d\n", error);
	}
}

//�¶ϵĵط�
LPVOID g_dwMemAddr;
void debugger::GetInput()
{
	char szCmd[0x10] = { 0 };
	while (1)
	{
		scanf_s("%s", szCmd, 0x10);
		if (!_stricmp(szCmd, "bp"))
		{
			DWORD addr = 0;
			scanf_s("%x", &addr);
			Breakpoint::SetInt3BreakPoint(hProcess, (LPVOID)addr);
		}
		else if (!_stricmp(szCmd, "t"))
		{
			flag = 1;
			Breakpoint::SetTfBreakPoint(hThread);
			break;
		}
		else if (!_stricmp(szCmd, "g"))
		{
			break;
		}
		else if (!_stricmp(szCmd, "ba"))
		{
			//ԭ����CPU�ṩ��Drϵ�мĴ��������������4��Ӳ���ϵ㣬
			//�ϵ��λ����Dr0~Dr3ȥ���棬��Ӧ��Dr7�е�Ln��ʾ��Ӧ��
			//�ϵ��Ƿ���Ч,Dr7�Ĵ������ṩ��RW\LEN��־λ,��������
			//�ϵ������,
			//RW:0(ִ�жϵ�,����lenҲ����Ϊ0���� 1(д) 3(��д��
			//len:0(1�ֽ�), 1��2�ֽ�), 2��8�ֽ�), 3��4�ֽ�)
			DWORD addr = 0;
			scanf_s("%x", &addr);
			int type;
			int len, l;
			scanf_s("%d", &type);
			scanf_s("%d", &len);
			if (len == 1)
				l = 0;
			else if (len == 2)
				l = 1;
			else if (len == 8)
				l = 2;
			else if (len == 4)
				l = 3;
			//Ӳ��ִ�жϵ�
			Breakpoint::SetHwBreakPoint(hThread, (LPVOID)addr, type, l);
		}
		else if (!_stricmp(szCmd, "mp"))
		{
			//0��ȡ�쳣��1д���쳣 8ִ���쳣
			DWORD addr;
			int type;
			scanf_s("%x%d", &addr, &type);
			g_dwMemAddr = (LPVOID)addr;
			Breakpoint::SetMmBreakPoint(hProcess, (LPVOID)addr, type);
		}
		else if (!_stricmp(szCmd, "Sym"))
		{
			DWORD addr;
			scanf_s("%x", &addr);
			getSymFromAddr(pi.hProcess, addr);
		}
	}
}