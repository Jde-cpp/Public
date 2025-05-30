#ifndef DECLARE_HANDLE
	#define DECLARE_HANDLE(name) struct name##__{int unused;}; typedef struct name##__ *name
	DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
#endif

int __stdcall DllMain( HMODULE /*hModule*/, unsigned long  ul_reason_for_call, void* /*lpReserved*/ ){
	switch (ul_reason_for_call){
	case 1://DLL_PROCESS_ATTACH
	case 2://DLL_THREAD_ATTACH
	case 3://DLL_THREAD_DETACH
	case 0://DLL_PROCESS_DETACH
		break;
	}
	return 1;
}

