/*+===================================================================
  File:      COMenum.exe

  Summary:   Enumerate the current user's CLSID registry keys to 
			 obtain COM interface information up to a specified
			 depth.

			 Algorithm:
			 1. Enumarete CLSID keys
			 2. Instantiate and Query info for each CLSID

  Classes:   Classes declared or used (in source files).

  Functions: Functions exported (in source files).

  Origin:    Indications of where content may have come from. This
			 is not a change history but rather a reference to the
			 editor-inheritance behind the content or other
			 indications about the origin of the source.

// References: 
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-value-types
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-reggetvaluea
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regqueryvalueexa?redirectedfrom=MSDN
//	https://docs.microsoft.com/en-us/windows/win32/com/com-class-objects-and-clsids
//	https://www.fireeye.com/blog/threat-research/2019/06/hunting-com-objects.html
//	https://docs.microsoft.com/en-us/windows/win32/api/winreg/nf-winreg-regopenkeya
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/registry-key-security-and-access-rights
//	https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cogetclassobject
//	http://forums.codeguru.com/showthread.php?194178-CLSID-IID-from-PROGID
//
// Examples:
//	https://stackoverflow.com/questions/820846/regopenkeyex-fails-on-hkey-local-machine
//	https://stackoverflow.com/questions/19255066/delete-all-values-under-a-registry-key-without-deleting-the-sub-key/19255134#19255134
//	https://docs.microsoft.com/en-us/windows/win32/sysinfo/enumerating-registry-subkeys?redirectedfrom=MSDN
//	http://www.cplusplus.com/forum/windows/45454/

===================================================================+*/

#include "pch.h"
#include <Windows.h>
#include <malloc.h>
#include <stdio.h>
#include <vector>
#include <tchar.h>
#include <atlbase.h>
#include <comdef.h>

// Quickly check the return code
inline void CHECK_CODE(DWORD retCode)
{
	if (retCode != ERROR_SUCCESS)
		printf("ERROR: code %d", retCode);
}

// Print a program information banner
void DisplayBanner()
{
	printf(
		"COMenum: enumerate Windows system for Component Object Model interfaces\n"
		" ++---++  / * -----------------------------------------------------------"
		" ||COM||    Usage: COMenum.exe <depth>\n"			
		" ++---++    Authors: Daniel Bloom, Joshua Finley\n"
		"           ----------------------------------------------------------- * /\n\n");
#ifdef NDEBUG
	Sleep(1500)
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

void InstantiateCOMObject(CLSID clsid, _Out_ void ** ppObjectPointer)
{
	USES_CONVERSION;
	LPOLESTR sCLSID;
	StringFromCLSID(clsid, &sCLSID);
	
	HRESULT retCode = CoCreateInstance(
		clsid,
		NULL,
		CLSCTX_ALL,
		IID_IUnknown,
		ppObjectPointer
	);

	_com_error err(retCode);
	LPCTSTR errMsg = err.ErrorMessage();

	if (retCode == S_OK)
	{
		wprintf(L"[+] %s instantiated successfully\n", sCLSID);
	}
	else if (retCode == REGDB_E_CLASSNOTREG)
	{
		wprintf(L"[ERROR] %s not registered in registration database or not registered for server type\n", sCLSID);
	}
	else if (retCode == E_NOINTERFACE)
	{
		wprintf(L"[ERROR] %s does not implement IUnkown interface\n", sCLSID);
	}
	else if (retCode == E_POINTER)
	{
		wprintf(L"[ERROR] %s pointer returned null\n", sCLSID);
	}
	else
	{
		wprintf(L"[ERROR] %s : unkown error: %s\n", sCLSID, err);
	}
}

HRESULT initializeCOM()
{
	HRESULT hr;
	hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		wprintf(
			L"Failed to initialize COM library. Error code = 0x%s",
			hr);
		return hr;
	}
}

int main()
{
	DisplayBanner();

	// Enumerate the local CLSIDs
	HKEY hCLSID_key;
	std::vector<CLSID> aClsidAll;
	
	if (RegOpenKeyEx(
		HKEY_CLASSES_ROOT,
		TEXT("CLSID\\"),
		0,
		KEY_READ,
		&hCLSID_key) == ERROR_SUCCESS)
	{
		QueryAllCOM(hCLSID_key, &aClsidAll);
	}

	RegCloseKey(hCLSID_key);

	// Instantiate the enumerated CLSIDs
	initializeCOM();

	std::vector<LPVOID> aPtrInstantiatedObjects;
	LPVOID lpBuf = new LPVOID;

	for (int i = 0; i < aClsidAll.size(); i++)
	{
		InstantiateCOMObject(aClsidAll[i], &lpBuf);
		aPtrInstantiatedObjects.push_back(lpBuf);
	}
}
