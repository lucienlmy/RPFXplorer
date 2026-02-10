#include "pch.h"
#include "CRAGEFolder.h"
#include "RPFPIDL.h"
#include "CRPFDataObject.h"
#include <shlwapi.h>

HRESULT CreateRPFEnumIDL(IRPF* pRPF, REFIID riid, LPVOID* ppvOut);

//-----------------------------------------------------------
// IContextMenu
//
STDMETHODIMP CRAGEFolder::QueryContextMenu(_In_ HMENU hMenu, _In_ UINT uIndexMenu, _In_ UINT uIDCmdFirst, _In_ UINT uIDCmdLast, _In_ UINT uFlags)
{
	if (!hMenu || hMenu == INVALID_HANDLE_VALUE)
		return E_HANDLE;

	// Don't add menu items if this is for the default verb
	if (uFlags & CMF_DEFAULTONLY)
		return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 0);

	// Add "Extract" menu item
	WCHAR szMenuText[] = L"Extract to Folder...";
	MENUITEMINFO mii = { sizeof(MENUITEMINFO) };
	mii.fMask = MIIM_STRING | MIIM_ID | MIIM_STATE;
	mii.wID = uIDCmdFirst;
	mii.fState = MFS_ENABLED;
	mii.dwTypeData = szMenuText;
	mii.cch = (UINT)wcslen(mii.dwTypeData);

	if (!InsertMenuItem(hMenu, uIndexMenu, TRUE, &mii))
		return HRESULT_FROM_WIN32(GetLastError());

	// Return number of menu items added
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

STDMETHODIMP CRAGEFolder::InvokeCommand(_In_ CMINVOKECOMMANDINFO* lpInfo)
{
	if (lpInfo == NULL)
		return E_POINTER;

	// Check if this is a string command
	if (HIWORD(lpInfo->lpVerb))
		return E_INVALIDARG;

	// Get command index
	UINT cmdIndex = LOWORD(lpInfo->lpVerb);

	switch (cmdIndex)
	{
	case 0: // Extract
	{
		// Show folder browser dialog
		BROWSEINFO bi = { 0 };
		bi.hwndOwner = lpInfo->hwnd;
		bi.lpszTitle = L"Select folder to extract files to:";
		bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

		LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
		if (pidl)
		{
			WCHAR szPath[MAX_PATH];
			if (SHGetPathFromIDList(pidl, szPath))
			{
				// TODO: Extract all files to the selected folder
				MessageBox(lpInfo->hwnd, L"Extract functionality will be implemented here", L"RPFXplorer", MB_OK | MB_ICONINFORMATION);
			}
			CoTaskMemFree(pidl);
		}

		return S_OK;
	}

	default:
		return E_INVALIDARG;
	}
}

STDMETHODIMP CRAGEFolder::GetCommandString(_In_ UINT_PTR pIDCmd, _In_ UINT uType, _Reserved_ UINT* pReserved, _In_ CHAR* pszName, _In_ UINT cchMax)
{
	if (pIDCmd != 0)
		return E_INVALIDARG;

	switch (uType)
	{
	case GCS_HELPTEXTW:
		wcscpy_s((LPWSTR)pszName, cchMax, L"Extract files from RPF archive");
		return S_OK;

	case GCS_VERBW:
		wcscpy_s((LPWSTR)pszName, cchMax, L"extract");
		return S_OK;

	default:
		return E_NOTIMPL;
	}
}

//-----------------------------------------------------------
// IShellExtInit
//
STDMETHODIMP CRAGEFolder::Initialize(_In_opt_ PCIDLIST_ABSOLUTE pidlFolder, _In_opt_ IDataObject* pdtobj, _In_opt_ HKEY hkeyProgID)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------
// IShellFolder
//
STDMETHODIMP CRAGEFolder::ParseDisplayName(_In_opt_ HWND hWnd, _In_opt_ LPBC pbc, _In_ LPOLESTR pszDisplayName, _Reserved_ LPDWORD pchEaten, _Out_ LPITEMIDLIST* ppidl, _Inout_ LPDWORD pdwAttributes)
{
	if (pszDisplayName == NULL || ppidl == NULL)
		return E_POINTER;

	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::EnumObjects(_In_opt_ HWND hWnd, _In_ DWORD grfFlags, _Out_opt_ IEnumIDList** ppenumIDList)
{
	return CreateRPFEnumIDL(this, IID_IEnumIDList, (void**)ppenumIDList);
}

STDMETHODIMP CRAGEFolder::BindToObject(_In_ PCUIDLIST_RELATIVE pidl, _In_opt_ LPBC pbc, _In_ REFIID riid, _Out_opt_ void** ppv)
{
	if (!pidl || !ppv)
		return E_POINTER;

	*ppv = NULL;

	// Get the RPF entry from the PIDL
	const RPFPidlData* pData = _PidlToRPFEntry(pidl);
	if (!pData)
		return E_INVALIDARG;

	// Only directories can be bound
	if (pData->rpfEntry.dwType != RPF_ENTRY_TYPE_DIRECTORY)
		return E_FAIL;

	// Create a new CRAGEFolder instance for the subdirectory
	CComObject<CRAGEFolder>* pNewFolder = NULL;
	HRESULT hr = CComObject<CRAGEFolder>::CreateInstance(&pNewFolder);
	if (FAILED(hr))
		return hr;

	pNewFolder->AddRef();

	// Initialize as a subfolder
	hr = pNewFolder->InitializeSubFolder(this, &pData->rpfEntry);
	if (FAILED(hr))
	{
		pNewFolder->Release();
		return hr;
	}

	// Query for the requested interface
	hr = pNewFolder->QueryInterface(riid, ppv);
	pNewFolder->Release();

	return hr;
}

STDMETHODIMP CRAGEFolder::BindToStorage(_In_ PCUIDLIST_RELATIVE pidl, _In_opt_ LPBC pbc, _In_ REFIID riid, _Out_opt_ void** ppv)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::CompareIDs(_In_ LPARAM lParam, _In_ PCUIDLIST_RELATIVE pidl1, _In_ PCUIDLIST_RELATIVE pidl2)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::CreateViewObject(_In_opt_ HWND hWndOwner, _In_ REFIID riid, _In_opt_ void** ppv)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::GetAttributesOf(_In_ UINT cidl, _In_ PCUITEMID_CHILD_ARRAY apidl, _Inout_ LPDWORD rgfInOut)
{
	if (!rgfInOut)
		return E_POINTER;

	if (cidl == 0 || !apidl)
		return E_INVALIDARG;

	DWORD dwAttribs = *rgfInOut;

	for (UINT i = 0; i < cidl; i++)
	{
		const RPFPidlData* pData = _PidlToRPFEntry(apidl[i]);
		if (!pData)
			continue;

		// Set attributes based on entry type
		if (pData->rpfEntry.dwType == RPF_ENTRY_TYPE_DIRECTORY)
		{
			dwAttribs &= SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_BROWSABLE | SFGAO_CANCOPY;
		}
		else
		{
			// Files can be copied, moved, and have properties
			dwAttribs &= SFGAO_CANCOPY | SFGAO_CANMOVE | SFGAO_HASPROPSHEET;
		}
	}

	*rgfInOut = dwAttribs;
	return S_OK;
}

STDMETHODIMP CRAGEFolder::GetUIObjectOf(_In_opt_ HWND hWndOwner, _In_ UINT cidl, _In_ PCUITEMID_CHILD_ARRAY apidl, _In_ REFIID riid, _Reserved_ LPUINT rgfReserved, _Out_opt_ void** ppv)
{
	if (!ppv)
		return E_POINTER;

	*ppv = NULL;

	if (cidl == 0 || !apidl)
		return E_INVALIDARG;

	// Check if requesting IDataObject (for drag-drop and copy operations)
	if (IsEqualIID(riid, IID_IDataObject))
	{
		// Get the RPF reader
		RPFReader* pReader = NULL;
		HRESULT hr = GetRPFReader(&pReader);
		if (FAILED(hr) || !pReader)
			return E_FAIL;

		// Create data object
		CComObject<CRPFDataObject>* pDataObj = NULL;
		hr = CComObject<CRPFDataObject>::CreateInstance(&pDataObj);
		if (FAILED(hr))
			return hr;

		pDataObj->AddRef();

		hr = pDataObj->Initialize(pReader, cidl, apidl);
		if (FAILED(hr))
		{
			pDataObj->Release();
			return hr;
		}

		hr = pDataObj->QueryInterface(riid, ppv);
		pDataObj->Release();

		return hr;
	}

	// Check if requesting IContextMenu
	if (IsEqualIID(riid, IID_IContextMenu))
	{
		// For now, return the folder itself as it implements IContextMenu
		return QueryInterface(riid, ppv);
	}

	return E_NOINTERFACE;
}

STDMETHODIMP CRAGEFolder::GetDisplayNameOf(_In_opt_ PCUITEMID_CHILD pidl, _In_ DWORD uFlags, _Out_ LPSTRRET pName)
{
	if (!pName)
		return E_POINTER;

	if (!pidl)
	{
		// Return the name of the RPF file itself
		pName->uType = STRRET_WSTR;
		return SHStrDup(m_strFilePath, &pName->pOleStr);
	}

	// Get the RPF entry from the PIDL
	const RPFPidlData* pData = _PidlToRPFEntry(pidl);
	if (!pData)
		return E_INVALIDARG;

	// Return the entry name
	pName->uType = STRRET_WSTR;
	return SHStrDup(pData->szName, &pName->pOleStr);
}

STDMETHODIMP CRAGEFolder::SetNameOf(_In_opt_ HWND hWnd, _In_ PCUITEMID_CHILD pidl, _In_ LPCWSTR pszName, _In_ SHGDNF uFlags, _Out_opt_ PITEMID_CHILD* ppidlOut)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------
// IShellFolder2
//
STDMETHODIMP CRAGEFolder::GetDefaultSearchGUID(_Out_ LPGUID pguid)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::EnumSearches(_Out_opt_ IEnumExtraSearch** ppenum)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::GetDefaultColumn(_In_ DWORD dwRes, _Out_ ULONG* pSort, _Out_ ULONG* pDisplay)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::GetDefaultColumnState(_In_ UINT iColumn, _Out_ SHCOLSTATEF* pcsFlags)
{
	if (!pcsFlags)
		return E_POINTER;

	static const SHCOLSTATEF columnStates[] = {
		SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  // Name
		SHCOLSTATE_TYPE_INT | SHCOLSTATE_ONBYDEFAULT,  // Size
		SHCOLSTATE_TYPE_STR | SHCOLSTATE_ONBYDEFAULT,  // Type
	};

	if (iColumn >= ARRAYSIZE(columnStates))
		return E_FAIL;

	*pcsFlags = columnStates[iColumn];
	return S_OK;
}

STDMETHODIMP CRAGEFolder::GetDetailsEx(_In_opt_ PCUITEMID_CHILD pidl, _In_ const SHCOLUMNID* pscid, _Out_ VARIANT* pv)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRAGEFolder::GetDetailsOf(_In_opt_ PCUITEMID_CHILD pidl, _In_ UINT iColumn, _In_ SHELLDETAILS* psd)
{
	if (!psd)
		return E_POINTER;

	// Column definitions
	static const struct {
		int cxChar;
		DWORD fmt;
		LPCWSTR pszTitle;
	} columns[] = {
		{ 30, LVCFMT_LEFT,  L"Name" },
		{ 15, LVCFMT_RIGHT, L"Size" },
		{ 20, LVCFMT_LEFT,  L"Type" },
	};

	if (iColumn >= ARRAYSIZE(columns))
		return E_FAIL;

	psd->fmt = columns[iColumn].fmt;
	psd->cxChar = columns[iColumn].cxChar;

	if (!pidl)
	{
		// Return column header
		psd->str.uType = STRRET_WSTR;
		return SHStrDup(columns[iColumn].pszTitle, &psd->str.pOleStr);
	}

	// Get the RPF entry
	const RPFPidlData* pData = _PidlToRPFEntry(pidl);
	if (!pData)
		return E_INVALIDARG;

	psd->str.uType = STRRET_WSTR;

	switch (iColumn)
	{
	case 0: // Name
		return SHStrDup(pData->szName, &psd->str.pOleStr);

	case 1: // Size
	{
		if (pData->rpfEntry.dwType == RPF_ENTRY_TYPE_DIRECTORY)
		{
			return SHStrDup(L"", &psd->str.pOleStr);
		}
		else
		{
			DWORD size = m_Reader.GetSizeFromEntry(&pData->rpfEntry);
			WCHAR szSize[32];
			StrFormatByteSize(size, szSize, ARRAYSIZE(szSize));
			return SHStrDup(szSize, &psd->str.pOleStr);
		}
	}

	case 2: // Type
	{
		if (pData->rpfEntry.dwType == RPF_ENTRY_TYPE_DIRECTORY)
		{
			return SHStrDup(L"File Folder", &psd->str.pOleStr);
		}
		else if (pData->rpfEntry.dwType == RPF_ENTRY_TYPE_RESOURCE)
		{
			return SHStrDup(L"Resource File", &psd->str.pOleStr);
		}
		else if (pData->rpfEntry.dwType == RPF_ENTRY_TYPE_BINARY)
		{
			return SHStrDup(L"Binary File", &psd->str.pOleStr);
		}
		else
		{
			return SHStrDup(L"Unknown", &psd->str.pOleStr);
		}
	}

	default:
		return E_FAIL;
	}
}

STDMETHODIMP CRAGEFolder::MapColumnToSCID(_In_ UINT, _Out_ SHCOLUMNID*)
{
	return E_NOTIMPL;
}

//-----------------------------------------------------------
// IPersist
//
STDMETHODIMP CRAGEFolder::GetClassID(_Out_ CLSID* pClassID)
{
	if (!pClassID)
		return E_POINTER;

	*pClassID = CLSID_RPFXplorerStorageHandler;
	return S_OK;
}

//-----------------------------------------------------------
// IPersistFolder
//
STDMETHODIMP CRAGEFolder::Initialize(_In_ PCIDLIST_ABSOLUTE pidl)
{
	if (!pidl)
		return E_INVALIDARG;

	// Store the PIDL
	if (m_pidl)
		CoTaskMemFree(m_pidl);

	m_pidl = ILClone(pidl);
	if (!m_pidl)
		return E_OUTOFMEMORY;

	// Get the file path from the PIDL
	WCHAR szPath[MAX_PATH];
	if (!SHGetPathFromIDList(pidl, szPath))
		return E_FAIL;

	m_strFilePath = szPath;

	// Open the RPF file
	m_hFile = CreateFile(m_strFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_hFile == INVALID_HANDLE_VALUE)
		return E_FAIL;

	// Initialize the RPF reader
	HRESULT hr = m_Reader.Open(m_hFile);
	if (FAILED(hr))
	{
		CloseHandle(m_hFile);
		m_hFile = NULL;
		return hr;
	}

	return S_OK;
}

//-----------------------------------------------------------
// IPersistFolder2
//
STDMETHODIMP CRAGEFolder::GetCurFolder(_Out_ PIDLIST_ABSOLUTE* ppidl)
{
	if (!ppidl)
		return E_POINTER;

	*ppidl = NULL;

	if (m_pidl)
	{
		*ppidl = ILClone(m_pidl);
		return *ppidl ? S_OK : E_OUTOFMEMORY;
	}

	return E_FAIL;
}

//-----------------------------------------------------------
// IRPF
//
STDMETHODIMP CRAGEFolder::GetRPFReader(_Out_ RPFReader** ppReader)
{
	if (!ppReader)
		return E_POINTER;

	// Return parent's reader if we're a subfolder
	if (m_bIsSubFolder && m_pParentFolder)
	{
		return m_pParentFolder->GetRPFReader(ppReader);
	}

	*ppReader = &m_Reader;
	return S_OK;
}

// Helper to get the current directory entry
RPFEntry* CRAGEFolder::GetCurrentDirectory()
{
	return m_pCurrentDirectory;
}
