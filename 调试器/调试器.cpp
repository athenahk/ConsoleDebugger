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
		debug.open("111.exe");
	else if (choice == 2)
	{

	}
	debug.run();
	return 0;
}