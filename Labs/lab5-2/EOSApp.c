#include "EOSApp.h"

//
// main 函数参数的意义：
// argc - argv 数组的长度，大小至少为 1，argc - 1 为命令行参数的数量。
// argv - 字符串指针数组，数组长度为命令行参数个数 + 1。其中 argv[0] 固定指向当前
//        进程所执行的可执行文件的路径字符串，argv[1] 及其后面的指针指向各个命令行
//        参数。
//        例如通过命令行内容 "a:\hello.exe -a -b" 启动进程后，hello.exe 的 main 函
//        数的参数 argc 的值为 3，argv[0] 指向字符串 "a:\hello.exe"，argv[1] 指向
//        参数字符串 "-a"，argv[2] 指向参数字符串 "-b"。
//
int main(int argc, char* argv[])
{
	//
	// 如果需要在调试应用程序时能够调试进入内核并显示对应的源码，
	// 必须使用 EOS 内核项目编译生成完全版本的 SDK 文件夹，然
	// 后使用刚刚生成的 SDK 文件夹覆盖此应用程序项目中的 SDK 文件
	// 夹，并且 EOS 内核项目在磁盘上的位置不能改变。
	//

	
	// 1. 调用API函数`VirtualAlloc`，分配一个整型变量所需的空间，并使用一个整型变量的指针指向这个空间。
	PINT space = VirtualAlloc(0, sizeof(INT), MEM_RESERVE|MEM_COMMIT);
	// 2. 修改整型变量的值为`0xFFFFFFFF`。在修改前输出整型变量的值，在修改后再输出整型变量的值。
	printf("Before: 0x%X\n", *space);
	*space = 0xFFFFFFFF;
	printf("After: 0x%X", *space);
	// 3. 调用API函数`Sleep`，等待10秒钟。
	Sleep(1000);
	// 4. 调用API函数`VirtualFree`，释放之前分配的整型变量的空间。
	VirtualFree(space, 0, MEM_RELEASE);
	// 5. 进入死循环，这样应用程序就不会结束。
	while(1);
	return 0;
}
