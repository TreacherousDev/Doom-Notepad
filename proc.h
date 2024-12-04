#include <windows.h>
#include <TlHelp32.h>

uintptr_t GetModuleBaseAddress(DWORD procId, const char* modName)
{
    //initialize to zero for error checking
    uintptr_t modBaseAddr = 0;

    //get a handle to a snapshot of all modules
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    //check if it's valid
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        //this struct holds the actual module information
        MODULEENTRY32 modEntry;

        //this is required for the function to work
        modEntry.dwSize = sizeof(modEntry);

        //If a module exists, get it's entry
        if (Module32First(hSnap, &modEntry))
        {
            //loop through the modules
            do
            {
                //compare the module name against ours

                {
                    //copy the base address and break out of the loop
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }

                //each iteration we grab the next module entry
            } while (Module32Next(hSnap, &modEntry));
        }
    }

    //free the handle
    CloseHandle(hSnap);
    return modBaseAddr;
}






DWORD FindProcessIdByName(const wchar_t* processName) {
    PROCESSENTRY32W entry; // Use the wide-character version of PROCESSENTRY32
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (Process32FirstW(snapshot, &entry)) { // Use Process32FirstW
        do {
            if (wcscmp(entry.szExeFile, processName) == 0) { // Use wcscmp for wide-character comparison
                CloseHandle(snapshot);
                return entry.th32ProcessID;
            }
        } while (Process32NextW(snapshot, &entry)); // Use Process32NextW
    }

    CloseHandle(snapshot);
    // Process not found
}