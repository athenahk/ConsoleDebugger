#include"breakpoint.h"
#include"debugger.h"
#include"capstone.h"
#include<vector>
using std::vector;
vector<bp> g_bp;
PreviousbpInfo bpInfo;

LPVOID oep;
typedef struct _DBG_REG7 {
	unsigned L0 : 1; unsigned G0 : 1;
	unsigned L1 : 1; unsigned G1 : 1;
	unsigned L2 : 1; unsigned G2 : 1;
	unsigned L3 : 1; unsigned G3 : 1;
	unsigned LE : 1; unsigned GE : 1;
	unsigned : 6;// 保留的无效空间
	unsigned RW0 : 2; unsigned LEN0 : 2;
	unsigned RW1 : 2; unsigned LEN1 : 2;
	unsigned RW2 : 2; unsigned LEN2 : 2;
	unsigned RW3 : 2; unsigned LEN3 : 2;
} R7, * PR7;

void Breakpoint::SetInt3BreakPoint(HANDLE hProcess, LPVOID addr)
{
	bp int3Bp = { 0 };
	int3Bp.type = 0;
	int3Bp.addr = addr;
	DWORD dwRead = 0;
	ReadProcessMemory(hProcess, addr, &int3Bp.oldBytes, 1, &dwRead);
	WriteProcessMemory(hProcess, addr, "\xcc", 1, &dwRead);
	g_bp.push_back(int3Bp);

}

void Breakpoint::SetTfBreakPoint(HANDLE hThread)
{
	CONTEXT context{ CONTEXT_ALL };
	GetThreadContext(hThread, &context);

	context.EFlags |= 0x00000100;
	SetThreadContext(hThread, &context);
}

DWORD Breakpoint::FixInt3BreakPoint(HANDLE hProcess, LPVOID addr, HANDLE hThread)
{
	for (int i = 0; i < g_bp.size(); i++)
	{
		if (addr == g_bp[i].addr && g_bp[i].type == 0)
		{
			bpInfo.PreviousAddr = addr;
			DWORD bytes = 0;
			WriteProcessMemory(hProcess, addr, &g_bp[i].oldBytes, 1, &bytes);

			CONTEXT context{ CONTEXT_ALL };
			GetThreadContext(hThread, &context);
			context.Eip -= 1;
			SetThreadContext(hThread, &context);
			if (oep != addr)
			{
				Breakpoint::SetTfBreakPoint(hThread);
				bpInfo.previouseIsBp = 1;
				bpInfo.whichbp = 0;
			}
			return DBG_CONTINUE;
		}
	}
	return DBG_EXCEPTION_NOT_HANDLED;
}

void Breakpoint::SetHwBreakPoint(HANDLE hThread, LPVOID addr, int type, int len)
{
	if (len == 1)
		addr = (LPVOID)((DWORD)addr - (DWORD)addr % 2);
	else if (len == 3)
		addr = (LPVOID)((DWORD)addr - (DWORD)addr % 4);
	CONTEXT context{ CONTEXT_ALL };
	GetThreadContext(hThread, &context);
	PR7 Dr7 = (PR7)&context.Dr7;

	if (Dr7->L0 == 0)
	{
		Dr7->L0 = 1;
		Dr7->LEN0 = len;
		Dr7->RW0 = type;
		context.Dr0 = (DWORD)addr;
	}
	else if (Dr7->L1 == 0)
	{
		Dr7->L1 = 1;
		Dr7->LEN1 = len;
		Dr7->RW1 = type;
		context.Dr1 = (DWORD)addr;
	}
	else if (Dr7->L2 == 0)
	{
		Dr7->L2 = 1;
		Dr7->LEN2 = len;
		Dr7->RW2 = type;
		context.Dr2 = (DWORD)addr;
	}
	else if (Dr7->L3 == 0)
	{
		Dr7->L3 = 1;
		Dr7->LEN3 = len;
		Dr7->RW3 = type;
		context.Dr3 = (DWORD)addr;
	}
	else
	{
		printf("没有可用的硬件断点寄存器!\n");
	}

	SetThreadContext(hThread, &context);

}

void Breakpoint::FixHwBreakPoint(HANDLE hThread, HANDLE hProcess, LPVOID& addr)
{
	CONTEXT context{ CONTEXT_ALL };
	GetThreadContext(hThread, &context);
	PR7 Dr7 = (PR7)&context.Dr7;
	LPVOID addr2 = 0;
	switch (context.Dr6 & 0xf)
	{
	case 1:Dr7->L0 = 0; bpInfo.type = Dr7->RW0; bpInfo.len = Dr7->LEN0; addr2 = (LPVOID)context.Dr0; break;
	case 2:Dr7->L1 = 0; bpInfo.type = Dr7->RW1; bpInfo.len = Dr7->LEN1; addr2 = (LPVOID)context.Dr1; break;
	case 4:Dr7->L2 = 0; bpInfo.type = Dr7->RW2; bpInfo.len = Dr7->LEN2; addr2 = (LPVOID)context.Dr2; break;
	case 8:Dr7->L3 = 0; bpInfo.type = Dr7->RW3; bpInfo.len = Dr7->LEN3; addr2 = (LPVOID)context.Dr3; break;
	}
	bpInfo.previouseIsBp = 1;
	bpInfo.whichbp = 1;
	if (bpInfo.type == 0)
	{
		bpInfo.PreviousAddr = addr;
	}
	else if (bpInfo.type == 1 || bpInfo.type == 3)
	{
		bpInfo.PreviousAddr = addr2;
	}

	SetThreadContext(hThread, &context);
	Breakpoint::SetTfBreakPoint(hThread);
}

DWORD dwOldProtected;
LPVOID addr;
int type;
extern LPVOID g_dwMemAddr;
extern int nIsGo;
void Breakpoint::FixMmBreakPoint(HANDLE hProcess, HANDLE hThread, LPVOID addr, int type)
{
	if (type == 8)
	{ 
		//断到想要断的地方
		if (g_dwMemAddr == addr)
			nIsGo = 1;
		VirtualProtectEx(hProcess, addr, 1, dwOldProtected, &dwOldProtected);
	}
	else if (type == 1)
	{
		if (g_dwMemAddr == addr)
		{
			nIsGo = 1;
		}
		VirtualProtectEx(hProcess, addr, 1, dwOldProtected, &dwOldProtected);
	}
	else if (type == 0)
	{
		if (g_dwMemAddr == addr)
		{
			nIsGo = 1;
		}
		VirtualProtectEx(hProcess, addr, 1, dwOldProtected, &dwOldProtected);
	}

	SetTfBreakPoint(hThread);
	bpInfo.previouseIsBp = 1;
	bpInfo.whichbp = 2;
}


void Breakpoint::SetMmBreakPoint(HANDLE hProcess, LPVOID addr, int type)
{
	//0读取异常，1写入异常 8执行异常
	if (type == 8)
	{
		::addr = addr;	
		::type = 8;
		VirtualProtectEx(hProcess, addr, 1, PAGE_READWRITE, &dwOldProtected);
	}
	else if (type == 1)
	{
		::addr = addr;
		::type = 1;
		VirtualProtectEx(hProcess, addr, 1, PAGE_EXECUTE_READ, &dwOldProtected);
	}
	else if (type == 0)
	{
		::addr = addr;
		::type = 0;
		VirtualProtectEx(hProcess, addr, 1, PAGE_NOACCESS, &dwOldProtected);

	}
}
