#include "pch.h"

#include <windows.h>
#include <fstream>
#include <iterator>
#include <chrono>
#include <thread>
#include <iostream>
#include <filesystem>
#include <vector>

// Check a modules exports for DllGetClassObject
// Returns
//		if the module possesses the export, ERROR_SUCCESS
//		if the module is not valid or PE parsing fails, ERROR_INVALID_DLL
//		otherwise, the file is not a COM DLL, returning ERROR_FUNCTION_FAILED
DWORD IsComModule(LPCWSTR lpModulePath) {
	// load the module
	HMODULE hModule = LoadLibraryEx(
		lpModulePath,
		NULL,
		LOAD_LIBRARY_AS_DATAFILE);
	if (hModule == NULL) return ERROR_INVALID_DLL;

	// extract the dos header
	auto pDosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(hModule);
	if (pDosHeader != NULL && pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return ERROR_INVALID_DLL;

	// extract the nt header
	auto pNtHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(
		reinterpret_cast<PBYTE>(hModule) + pDosHeader->e_lfanew);

	// extract the export directory
	auto pExports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
		(PBYTE)hModule +
		pNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	if (pExports->NumberOfFunctions < 0) return ERROR_INVALID_DLL;

	// set variables for export directory iterables
	auto pdwAddrs = reinterpret_cast<PDWORD>((LPBYTE)hModule + pExports->AddressOfFunctions);
	auto pdwNames = reinterpret_cast<PDWORD>((LPBYTE)hModule + pExports->AddressOfNames);
	auto pwOrds = reinterpret_cast<PWORD>((LPBYTE)hModule + pExports->AddressOfNameOrdinals);

	// loop over exports
	for (DWORD i = 0; i < pExports->NumberOfFunctions; ++i) {
		std::string funcname = (char*)hModule + pdwNames[i];

		if (strncmp(funcname.c_str(), (char*)"DLLGetClassObject", 18)) {
			return ERROR_SUCCESS;
		}
	}

	// DllGetClassObject was not found
	return ERROR_FUNCTION_FAILED;
}

int main()
{
	std::vector<std::string> vecComModules;

	ULONGLONG ullNumDlls = 0;
	for (std::filesystem::recursive_directory_iterator i("C:\\",
		std::filesystem::directory_options::skip_permission_denied), end; i != end; ++i)
	{
		try {
			if (!is_directory(i->path()))
				if (i->path().string().substr(i->path().string().find_last_of(".") + 1) == "dll")
				{
					ullNumDlls++;
					std::string strModulePath = i->path().string();
					std::wstring stemp = std::wstring(strModulePath.begin(), strModulePath.end());
					LPCWSTR sw = stemp.c_str();
					
					std::cout << "[" << ullNumDlls << "] Checking if " << strModulePath << " is a COM DLL\n";

					if (IsComModule((LPCWSTR)sw) == ERROR_SUCCESS) {
						std::cout << "[!] COM DLL found: "
							<< i->path().filename() << "\n";
						vecComModules.push_back(strModulePath);
						std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					}
				}
		}
		catch (std::filesystem::filesystem_error) {
			continue;
		}
	}

	std::ofstream output_file("./COM_DLLs.txt");
	std::copy(vecComModules.rbegin(), vecComModules.rend(),
		std::ostream_iterator<std::string>(output_file, "\n"));

	return 0;
}
