// COMEnum.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
// Enumerate the current user's CLSID registry keys to obtain COM interface information up to
// a specified depth.
//
// References: 
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-reggetvaluea
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexa?redirectedfrom=MSDN
//	https://docs.microsoft.com/en-us/windows/win32/com/com-class-objects-and-clsids
//	https://www.fireeye.com/blog/threat-research/2019/06/hunting-com-objects.html
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeya
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-key-security-and-access-rights
//
// Examples:
//	https://stackoverflow.com/questions/820846/regopenkeyex-fails-on-hkey-local-machine
//	https://stackoverflow.com/questions/19255066/delete-all-values-under-a-registry-key-without-deleting-the-sub-key/19255134#19255134
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/enumerating-registry-subkeys?redirectedfrom=MSDN
//	http://www.cplusplus.com/forum/windows/45454/

#include "pch.h"
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>
#include <vector>
#include <tchar.h>
#include <atlbase.h>
#pragma comment( lib, "rpcrt4.lib" )

// Quickly check the return code
inline void CHECK_CODE(DWORD retCode)
{
	if (ret_code != ERROR_SUCCESS)
		printf("ERROR: code %d", ret_code);
}

// Print a program information banner
void print_program_info()
{
	printf(
		"COMenum: enumerate Windows system for Component Object Model interfaces\n"
		" ++--++ /* Usage: COMenum.exe <depth>\n"
		" ++--++    Authors: Daniel Bloom, Joshua Finley */\n\n");
#ifdef NDEBUG
	Sleep(1500);
#endif
}

// Utility function to create a valid CLSID from a string
template<class S>
CLSID CreateGUID(const S& hexString)
{
	CLSID clsid;
	CLSIDFromString(CComBSTR(hexString), &clsid);

	return clsid;
}

// Query the registry for all COM objects
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383 

int QueryAllCOM(HKEY hKey, _Out_ std::vector<CLSID>* vData)
{
	printf("Querying CLSID key...\n");
#ifdef NDEBUG
	Sleep(1750);
#endif

	TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
	DWORD    cbName;                   // size of name string 
	TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
	DWORD    cchClassName = MAX_PATH;  // size of class string 
	DWORD    cSubKeys = 0;               // number of subkeys 
	DWORD    cbMaxSubKey;              // longest subkey size 
	DWORD    cchMaxClass;              // longest class string 
	DWORD    cValues;              // number of values for key 
	DWORD    cchMaxValue;          // longest value name 
	DWORD    cbMaxValueData;       // longest value data 
	DWORD    cbSecurityDescriptor; // size of security descriptor 
	FILETIME ftLastWriteTime;      // last write time 

	DWORD i, retCode;

	TCHAR  achValue[MAX_VALUE_NAME];
	DWORD cchValue = MAX_VALUE_NAME;

	// Get the class name and the value count. 
	retCode = RegQueryInfoKey(
		hKey,                    // key handle 
		achClass,                // buffer for class name 
		&cchClassName,           // size of class string 
		NULL,                    // reserved 
		&cSubKeys,               // number of subkeys 
		&cbMaxSubKey,            // longest subkey size 
		&cchMaxClass,            // longest class string 
		&cValues,                // number of values for this key 
		&cchMaxValue,            // longest value name 
		&cbMaxValueData,         // longest value data 
		&cbSecurityDescriptor,   // security descriptor 
		&ftLastWriteTime);       // last write time 

	// Enumerate the subkeys to a string buffer, until RegEnumKeyEx fails.
	printf("Enumerating subkeys...\n");
#ifdef NDEBUG
	Sleep(1750);
#endif

	// Create the CLSID vector for eventual return
	std::vector<CLSID>& clsidAll = *vData;

	if (cSubKeys)
	{
		printf("\nNumber of subkeys: %d\n", cSubKeys);

		for (i = 0; i < cSubKeys; i++)
		{
			cbName = MAX_KEY_LENGTH;
			retCode = RegEnumKeyEx(hKey, i,
				achKey,
				&cbName,
				NULL,
				NULL,
				NULL,
				&ftLastWriteTime);
			if (retCode == ERROR_SUCCESS)
			{
				// Add the CLSID to the return vector
				CLSID clsid = CreateGUID(achKey);
				clsidAll.push_back(clsid);

				_tprintf(TEXT("(%d) %s\n"), i + 1, achKey);
			}
		}
	}

	// Enumerate the key values. 

	if (cValues)
	{
		printf("\nNumber of values: %d\n", cValues);

		for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
		{
			
			cchValue = MAX_VALUE_NAME;
			achValue[0] = '\0';
			retCode = RegEnumValue(hKey, i,
				achValue,
				&cchValue,
				NULL,
				NULL,
				NULL,
				NULL);
			
			if (retCode == ERROR_SUCCESS)
			{
				DWORD lpData = cbMaxValueData;
				LONG dwRes = RegQueryValueEx(hKey, achValue, 0, NULL, NULL, &lpData);
				_tprintf(TEXT("(%d) %s \n"), i + 1, achValue);
			}
		}

		if (!clsidAll.empty())
		{
			return ERROR_EMPTY;
		}
		else
		{
			return ERROR_SUCCESS;
		}	
	}
}

int main()
{
	print_program_info();

	HKEY hCLSID_key;
	std::vector<CLSID> clsidAll;
	
	if (RegOpenKeyEx(
		HKEY_CLASSES_ROOT,
		TEXT("CLSID\\"),
		0,
		KEY_READ,
		&hCLSID_key) == ERROR_SUCCESS)
	{
		QueryAllCOM(hCLSID_key, &clsidAll);
	}

	RegCloseKey(hCLSID_key);
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
