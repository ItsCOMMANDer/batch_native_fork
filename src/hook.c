/* Licensed under conditions stated in the
   "getinput.txt" document you recieved with this software.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include "../bin/dllcode.h"
#include "extern/lz77.c"

// This is extremely ugly
// However its a) not mine and b) not getting edited again soon

// I somehow ported it to x64 and it works

typedef unsigned __int64 QWORD;

typedef struct {
    PBYTE imageBase;
    HMODULE(WINAPI* loadLibraryA)(PCSTR);
    FARPROC(WINAPI* getProcAddress)(HMODULE, PCSTR);
    VOID(WINAPI* rtlZeroMemory)(PVOID, SIZE_T);
} LoaderData;

QWORD WINAPI loadLibrary(LoaderData* loaderData) {
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(loaderData->imageBase + ((PIMAGE_DOS_HEADER)loaderData->imageBase)->e_lfanew);
    PIMAGE_BASE_RELOCATION relocation = (PIMAGE_BASE_RELOCATION)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
    QWORD delta = (QWORD)(loaderData->imageBase - ntHeaders->OptionalHeader.ImageBase);
    while (relocation->VirtualAddress) {
        PWORD relocationInfo = (PWORD)(relocation + 1);
        for (int i = 0, count = (relocation->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD); i < count; i++)
            if (relocationInfo[i] >> 12 == IMAGE_REL_BASED_HIGHLOW)
                *(QWORD *)(loaderData->imageBase + (relocation->VirtualAddress + (relocationInfo[i] & 0xFFF))) += delta;

        relocation = (PIMAGE_BASE_RELOCATION)((LPBYTE)relocation + relocation->SizeOfBlock);
    }

    PIMAGE_IMPORT_DESCRIPTOR importDirectory = (PIMAGE_IMPORT_DESCRIPTOR)(loaderData->imageBase + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    while (importDirectory->Characteristics) {
        PIMAGE_THUNK_DATA originalFirstThunk = (PIMAGE_THUNK_DATA)(loaderData->imageBase + importDirectory->OriginalFirstThunk);
        PIMAGE_THUNK_DATA firstThunk = (PIMAGE_THUNK_DATA)(loaderData->imageBase + importDirectory->FirstThunk);

        HMODULE module = loaderData->loadLibraryA((LPCSTR)loaderData->imageBase + importDirectory->Name);

        if (!module)
            return FALSE;

        while (originalFirstThunk->u1.AddressOfData) {
            QWORD Function = (QWORD)loaderData->getProcAddress(module, originalFirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG ? (LPCSTR)(originalFirstThunk->u1.Ordinal & 0xFFFF) : ((PIMAGE_IMPORT_BY_NAME)((LPBYTE)loaderData->imageBase + originalFirstThunk->u1.AddressOfData))->Name);

            if (!Function)
                return FALSE;

            firstThunk->u1.Function = Function;
            originalFirstThunk++;
            firstThunk++;
        }
        importDirectory++;
    }

    if (ntHeaders->OptionalHeader.AddressOfEntryPoint) {
        QWORD result = ((DWORD(__stdcall*)(HMODULE, DWORD, LPVOID))
            (loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint))
            ((HMODULE)loaderData->imageBase, DLL_PROCESS_ATTACH, NULL);

        loaderData->rtlZeroMemory(loaderData->imageBase + ntHeaders->OptionalHeader.AddressOfEntryPoint, 32);
        loaderData->rtlZeroMemory(loaderData->imageBase, ntHeaders->OptionalHeader.SizeOfHeaders);
        return result;
    }
    return TRUE;
}

// only serves as a marker
void stub(void) {}

void HookDll(HANDLE hProcess, LPVOID dllcode) {
    PIMAGE_NT_HEADERS ntHeaders = (PIMAGE_NT_HEADERS)(dllcode + ((PIMAGE_DOS_HEADER)dllcode)->e_lfanew);
    PBYTE executableImage = VirtualAllocEx(hProcess, NULL, ntHeaders->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    WriteProcessMemory(hProcess, executableImage, dllcode, ntHeaders->OptionalHeader.SizeOfHeaders, NULL);

    PIMAGE_SECTION_HEADER sectionHeaders = (PIMAGE_SECTION_HEADER)(ntHeaders + 1);
    for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; i++)
        WriteProcessMemory(hProcess, executableImage + sectionHeaders[i].VirtualAddress, dllcode + sectionHeaders[i].PointerToRawData, sectionHeaders[i].SizeOfRawData, NULL);

    LoaderData* loaderMemory = VirtualAllocEx(hProcess, NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READ);

    LoaderData loaderParams = {
    	.imageBase = executableImage,
    	.loadLibraryA = GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"),
    	.getProcAddress = GetProcAddress,
    	.rtlZeroMemory = (VOID(NTAPI*)(PVOID, SIZE_T))GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlZeroMemory")
    };

    WriteProcessMemory(hProcess, loaderMemory, &loaderParams, sizeof(LoaderData), NULL);
    WriteProcessMemory(hProcess, loaderMemory + 1, loadLibrary, (QWORD)stub - (QWORD)loadLibrary, NULL);
    WaitForSingleObject(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)(loaderMemory + 1), loaderMemory, 0, NULL), INFINITE);
    VirtualFreeEx(hProcess, loaderMemory, 0, MEM_RELEASE);
}

VOID UncompressAndHook(HANDLE hProcess, HANDLE hProcHeap, LPVOID compressedDllCode, int compressedSize, int realSize) {
	char *buffer = HeapAlloc(hProcHeap, HEAP_ZERO_MEMORY, realSize+2);
	lz77_decompress(compressedDllCode, compressedSize, buffer, realSize+1);
	HookDll(hProcess, buffer);
	HeapFree(hProcHeap, 0, buffer);
}

// TODO error checking
INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd) {
	PROCESS_BASIC_INFORMATION ProcessInfo;
	if (!NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &ProcessInfo, sizeof(ProcessInfo), NULL)))
		return -3;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, ProcessInfo.InheritedFromUniqueProcessId);

	TCHAR parentPath[MAX_PATH];
	GetModuleFileNameEx(hProcess, NULL, parentPath, MAX_PATH);
	if(lstrcmp("cmd.exe", parentPath + lstrlen(parentPath) - 7) != 0) {
		MessageBox(0, "Parrent process must be cmd.exe!", "", MB_OK | MB_ICONERROR);
		return -2;
	}

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	HANDLE hProcHeap = GetProcessHeap();

	for (int i = 1; i <= argc; i++) {
		switch(argv[i][0]) {
		case 'i': UncompressAndHook(hProcess, hProcHeap, getinput_dll_code, getinput_size, getinput_real_size); break;
		case 'd': {
			if(GetEnvironmentVariable("discordappid", NULL, 0) == 0) break; // dont even bother
			UncompressAndHook(hProcess, hProcHeap, discord_dll_code, discord_size, discord_real_size);
		} break;
		default: break;
		}
	}

	CloseHandle(hProcHeap);
    return 0;
}
