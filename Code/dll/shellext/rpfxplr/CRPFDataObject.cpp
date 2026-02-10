#include "pch.h"
#include "CRPFDataObject.h"
#include <shlwapi.h>
#include <shlobj.h>

ATL::CString CRPFDataObject::GetTempPath()
{
	if (!m_strTempPath.IsEmpty())
		return m_strTempPath;

	WCHAR szTempPath[MAX_PATH];
	WCHAR szTempDir[MAX_PATH];

	// Get system temp path
	::GetTempPath(MAX_PATH, szTempPath);

	// Create a unique subdirectory for our extracted files
	wsprintf(szTempDir, L"%sRPFXplorer_%08X\\", szTempPath, GetTickCount());
	CreateDirectory(szTempDir, NULL);

	m_strTempPath = szTempDir;
	return m_strTempPath;
}

HRESULT CRPFDataObject::CreateTempFiles()
{
	ATL::CString tempPath = GetTempPath();

	for (const auto& entry : m_vEntries)
	{
		// Skip directories
		if (entry.dwType == RPF_ENTRY_TYPE_DIRECTORY)
			continue;

		// Get file name
		ATL::CString fileName = m_pReader->GetName(&entry);
		ATL::CString fullPath = tempPath + fileName;

		// Extract file content
		std::vector<BYTE> content = m_pReader->GetContent(&entry);
		if (content.empty())
			continue;

		// Write to temp file
		HANDLE hFile = CreateFile(fullPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			continue;

		DWORD written;
		WriteFile(hFile, content.data(), (DWORD)content.size(), &written, NULL);
		CloseHandle(hFile);
	}

	return S_OK;
}

void CRPFDataObject::CleanupTempFiles()
{
	if (m_strTempPath.IsEmpty())
		return;

	// Delete all files in the temp directory
	WIN32_FIND_DATA findData;
	ATL::CString searchPath = m_strTempPath + L"*.*";
	HANDLE hFind = FindFirstFile(searchPath, &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)
			{
				ATL::CString filePath = m_strTempPath + findData.cFileName;
				DeleteFile(filePath);
			}
		} while (FindNextFile(hFind, &findData));

		FindClose(hFind);
	}

	// Remove the directory
	RemoveDirectory(m_strTempPath);
	m_strTempPath.Empty();
}

STDMETHODIMP CRPFDataObject::GetData(_In_ FORMATETC* pformatetcIn, _Out_ STGMEDIUM* pmedium)
{
	if (!pformatetcIn || !pmedium)
		return E_POINTER;

	ZeroMemory(pmedium, sizeof(STGMEDIUM));

	// We only support CF_HDROP format
	if (pformatetcIn->cfFormat != CF_HDROP)
		return DV_E_FORMATETC;

	if (!(pformatetcIn->tymed & TYMED_HGLOBAL))
		return DV_E_TYMED;

	// Create temporary files
	HRESULT hr = CreateTempFiles();
	if (FAILED(hr))
		return hr;

	// Build file list
	std::vector<ATL::CString> fileList;
	ATL::CString tempPath = GetTempPath();

	for (const auto& entry : m_vEntries)
	{
		if (entry.dwType == RPF_ENTRY_TYPE_DIRECTORY)
			continue;

		ATL::CString fileName = m_pReader->GetName(&entry);
		ATL::CString fullPath = tempPath + fileName;
		fileList.push_back(fullPath);
	}

	if (fileList.empty())
		return E_FAIL;

	// Calculate size needed for DROPFILES structure
	size_t totalSize = sizeof(DROPFILES);
	for (const auto& file : fileList)
	{
		totalSize += (file.GetLength() + 1) * sizeof(WCHAR);
	}
	totalSize += sizeof(WCHAR); // Final null terminator

	// Allocate global memory
	HGLOBAL hGlobal = GlobalAlloc(GHND, totalSize);
	if (!hGlobal)
		return E_OUTOFMEMORY;

	DROPFILES* pDropFiles = (DROPFILES*)GlobalLock(hGlobal);
	if (!pDropFiles)
	{
		GlobalFree(hGlobal);
		return E_FAIL;
	}

	// Fill DROPFILES structure
	pDropFiles->pFiles = sizeof(DROPFILES);
	pDropFiles->pt.x = 0;
	pDropFiles->pt.y = 0;
	pDropFiles->fNC = FALSE;
	pDropFiles->fWide = TRUE;

	// Copy file paths
	WCHAR* pszFiles = (WCHAR*)((BYTE*)pDropFiles + sizeof(DROPFILES));
	for (const auto& file : fileList)
	{
		size_t remainingSize = (totalSize - ((BYTE*)pszFiles - (BYTE*)pDropFiles)) / sizeof(WCHAR);
		wcscpy_s(pszFiles, remainingSize, file.GetString());
		pszFiles += file.GetLength() + 1;
	}
	*pszFiles = L'\0'; // Final null terminator

	GlobalUnlock(hGlobal);

	// Fill STGMEDIUM
	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->hGlobal = hGlobal;
	pmedium->pUnkForRelease = NULL;

	return S_OK;
}

STDMETHODIMP CRPFDataObject::GetDataHere(_In_ FORMATETC* pformatetc, _Inout_ STGMEDIUM* pmedium)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRPFDataObject::QueryGetData(_In_opt_ FORMATETC* pformatetc)
{
	if (!pformatetc)
		return E_POINTER;

	if (pformatetc->cfFormat == CF_HDROP && (pformatetc->tymed & TYMED_HGLOBAL))
		return S_OK;

	return DV_E_FORMATETC;
}

STDMETHODIMP CRPFDataObject::GetCanonicalFormatEtc(_In_opt_ FORMATETC* pformatectIn, _Out_ FORMATETC* pformatetcOut)
{
	if (!pformatetcOut)
		return E_POINTER;

	pformatetcOut->ptd = NULL;
	return E_NOTIMPL;
}

STDMETHODIMP CRPFDataObject::SetData(_In_ FORMATETC* pformatetc, _In_ STGMEDIUM* pmedium, _In_ BOOL fRelease)
{
	return E_NOTIMPL;
}

STDMETHODIMP CRPFDataObject::EnumFormatEtc(_In_ DWORD dwDirection, _Out_opt_ IEnumFORMATETC** ppenumFormatEtc)
{
	if (!ppenumFormatEtc)
		return E_POINTER;

	*ppenumFormatEtc = NULL;

	if (dwDirection != DATADIR_GET)
		return E_NOTIMPL;

	// We only support CF_HDROP
	FORMATETC fmtetc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	return SHCreateStdEnumFmtEtc(1, &fmtetc, ppenumFormatEtc);
}

STDMETHODIMP CRPFDataObject::DAdvise(_In_ FORMATETC* pformatetc, _In_ DWORD advf, _In_opt_ IAdviseSink* pAdvSink, _Out_ DWORD* pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CRPFDataObject::DUnadvise(_In_ DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

STDMETHODIMP CRPFDataObject::EnumDAdvise(_Out_opt_ IEnumSTATDATA** ppenumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}
