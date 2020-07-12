//===============================================================================================//
// This is a stub for the actuall functionality of the DLL.
//===============================================================================================//
#include "ReflectiveLoader.h"
#include <string>
#include <shellapi.h>
#include <ctime>
#include <windows.h>
#include <stdio.h>
#pragma comment(lib, "Shell32.lib")
// Note: REFLECTIVEDLLINJECTION_VIA_LOADREMOTELIBRARYR and REFLECTIVEDLLINJECTION_CUSTOM_DLLMAIN are
// defined in the project properties (Properties->C++->Preprocessor) so as we can specify our own 
// DllMain and use the LoadRemoteLibraryR() API to inject this DLL.


// You can use this value as a pseudo hinstDLL value (defined and set via ReflectiveLoader.c)


std::string szargs;
std::wstring wszargs;
std::wstring wsHostFile;
int argc = 0;
LPWSTR* argv = NULL;

extern HINSTANCE hAppInstance;
//===============================================================================================//


std::wstring StringToWString(const std::string& str)
{
	int num = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
	wchar_t* wide = new wchar_t[num];
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wide, num);
	std::wstring w_str(wide);
	delete[] wide;
	return w_str;
}
BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpReserved ) {
	BOOL bReturnValue = TRUE;
	switch( dwReason ) {
		case DLL_QUERY_HMODULE:
			if( lpReserved != NULL )
				*(HMODULE *)lpReserved = hAppInstance;
			break;
		case DLL_PROCESS_ATTACH:
		{
			//利用命名管道传输
			srand(time(NULL));
			DWORD wlen = 0;

			BOOL bRet = WaitNamedPipe(TEXT("\\\\.\\Pipe\\mypipe"), NMPWAIT_WAIT_FOREVER);

			if (!bRet)
			{
				printf("connect the namedPipe failed!\n");
				break;
			}

			HANDLE hPipe = CreateFile(			//管道属于一种特殊的文件
				TEXT("\\\\.\\Pipe\\mypipe"),	//创建的文件名
				GENERIC_READ | GENERIC_WRITE,	//文件模式
				0,								//是否共享
				NULL,							//指向一个SECURITY_ATTRIBUTES结构的指针
				OPEN_EXISTING,					//创建参数
				FILE_ATTRIBUTE_NORMAL,			//文件属性(隐藏,只读)NORMAL为默认属性
				NULL);							//模板创建文件的句柄


			hAppInstance = hinstDLL;
			char buf[256] = "";
			sprintf(buf, "C++ ReflectiveDLL\n");
			WriteFile(hPipe, buf, sizeof(buf), &wlen, 0);	//向服务器发送内容

			/* print some output to the operator */
			if (lpReserved != NULL) {
				szargs = (PCHAR)lpReserved;
				wszargs = StringToWString(szargs);
				argv = CommandLineToArgvW(wszargs.data(), &argc);
			}
			else {
				sprintf(buf, "Hello from test.dll. There is no parameter\n");
				WriteFile(hPipe, buf, sizeof(buf), &wlen, 0);	//向服务器发送内容
			}
			if (argv == NULL) {
				sprintf(buf, "[+] Error Arguments ! \n");
				WriteFile(hPipe, buf, sizeof(buf), &wlen, 0);	//向服务器发送内容
				break;
			}
			sprintf(buf, "[+] Args Count : %d \n", argc);
			WriteFile(hPipe, buf, sizeof(buf), &wlen, 0);	//向服务器发送内容
			for (size_t i = 0; i < argc; i++)
			{
				sprintf(buf, "[%d] %s \n", i, argv[i]);
				WriteFile(hPipe, buf, sizeof(buf), &wlen, 0);	//向服务器发送内容
			}
				CloseHandle(hPipe);//关闭管道

				/* flush STDOUT */
				fflush(stdout);

				/* we're done, so let's exit */
				ExitProcess(0);
		}
			break;
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}
	return bReturnValue;
}