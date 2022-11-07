#include<stdio.h>
#include"debugger.h"
#include"capstone.h"

int main()
{
	debugger debug;
	int choice;
	printf("1.打开程序, 2.附加程序:");
	scanf_s("%d", &choice);
	if (choice == 1)
		debug.open("ConsoleApplication1.exe");
	else if (choice == 2)
	{
		Capstone::Init();
		DWORD pid;
		scanf_s("%d", &pid);
		DebugActiveProcess(pid);
	}
	debug.run();
	return 0;
}