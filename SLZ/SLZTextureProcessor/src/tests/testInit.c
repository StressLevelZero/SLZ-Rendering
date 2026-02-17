#include <stdio.h>
#include <windows.h>

int main(int argc, char* argv[])
{
	HINSTANCE lib = LoadLibrary("SLZTextureProcessor.dll");
	if (lib == NULL)
	{
		printf("Failed to load SLZTextureProcessor.dll");
		return 0;
	}
	//typedef int (__stdcall *PFN_InitializeLibPath)();
	//printf("Test\n");
	//PFN_InitializeLibPath InitializeLibPath = (PFN_InitializeLibPath)GetProcAddress(lib, "InitializeLibPath");
	//if (InitializeLibPath == NULL)
	//{
	//	printf("Failed to find InitializeLibPath in DLL!\n");
	//	return 0;
	//}
	//if (InitializeLibPath() != 0)
	//{
	//	printf("Failed to find dll path!\n");
	//	return 0;
	//}
	//printf("Printing library path?\n");
	//typedef void (__stdcall *PFN_PrintLibPath)();
	//PFN_PrintLibPath PrintLibPath = (PFN_PrintLibPath)GetProcAddress(lib, "PrintLibPath");
	//PrintLibPath();
	//
	//int result;
	//printf("Trying to find openCL devices?\n");
	//typedef int (__stdcall *PFN_TestOpenClLoad)();
	//PFN_TestOpenClLoad TestOpenClLoad = (PFN_TestOpenClLoad)GetProcAddress(lib, "TestOpenClLoad");
	//int result = TestOpenClLoad();
	//
	//printf("Trying to load openCL?\n");
	//typedef int(__stdcall* PFN_InitCLContext)();
	//PFN_InitCLContext InitCLContext = (PFN_InitCLContext)GetProcAddress(lib, "InitCLContext");
	//result = InitCLContext();
	// 
	//printf("Trying to release openCL context?\n");
	//typedef int(__stdcall* PFN_ReleaseCLContext)();
	//PFN_ReleaseCLContext ReleaseCLContext = (PFN_ReleaseCLContext)GetProcAddress(lib, "ReleaseCLContext");
	//result = ReleaseCLContext();

	//printf("Trying to open test image?\n");
	//typedef int(__stdcall* PFN_OpenTestImage)();
	//PFN_OpenTestImage OpenTestImage = (PFN_OpenTestImage)GetProcAddress(lib, "OpenTestImage");
	//result = OpenTestImage();
	printf("Testing sail library\n");
	typedef int(__stdcall* PFN_TestImageRead)();
	PFN_TestImageRead OpenTestImage = (PFN_TestImageRead)GetProcAddress(lib, "TestImageRead");
	OpenTestImage();


	FreeLibrary(lib);
	return 0;
}