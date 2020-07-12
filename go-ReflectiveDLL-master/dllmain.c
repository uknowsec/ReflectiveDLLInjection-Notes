#include "dllmain.h"
#include<Windows.h>


BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD fdwReason,     // reason for calling function
    LPVOID lpReserved)   // reserved
{
    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
        {
            GoString goArgs = {0};
            if(lpReserved != NULL){
                goArgs.p = (char*)lpReserved;
                goArgs.n = strlen(lpReserved);
            }else{
                goArgs.p = "";
                goArgs.n = 0;
            }
            test(goArgs);
        }
        break;
    case DLL_PROCESS_DETACH:
        // Perform any necessary cleanup.
        break;
    case DLL_THREAD_DETACH:
        // Do thread-specific cleanup.
        break;
    case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
        break;
    }
    return TRUE; // Successful.
}