#include<windows.h>

typedef struct _bp
{
	int type;          //0 int 3  1硬件   2内存
	LPVOID addr;       //断点地址
	BYTE oldBytes;
}bp;

struct PreviousbpInfo {
	int previouseIsBp = 0;
	int whichbp = -1;
	//上一条断点地址
	LPVOID PreviousAddr;
	int type;
	int len;
};
class Breakpoint
{
public:
	static void SetInt3BreakPoint(HANDLE hProcess, LPVOID addr);
	static void SetTfBreakPoint(HANDLE hThread);
	static DWORD FixInt3BreakPoint(HANDLE hProcess, LPVOID addr, HANDLE hThread);
	static void SetHwBreakPoint(HANDLE hThread, LPVOID addr, int type, int len);
	static void FixHwBreakPoint(HANDLE hThread, HANDLE hProcess, LPVOID& addr);
	static void FixMmBreakPoint(HANDLE hProcess, HANDLE hThread,LPVOID addr, int type);
	static void SetMmBreakPoint(HANDLE hProcess, LPVOID addr,int type);
};