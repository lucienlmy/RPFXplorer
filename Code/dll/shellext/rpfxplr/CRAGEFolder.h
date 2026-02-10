#pragma once

DEFINE_GUID(CLSID_RPFXplorerStorageHandler, 0xebc8c101, 0x27ae, 0x48d8, 0xbd, 0xb5, 0x22, 0x34, 0x51, 0xb4, 0xc2, 0xc2);
DEFINE_GUID(CLSID_RPFXplorerContextMenu, 0x8dc327c, 0x3815, 0x4dee, 0x9c, 0x84, 0xad, 0x5c, 0x80, 0xa4, 0x56, 0x81);

class ATL_NO_VTABLE CRAGEFolder :
	public CComObjectRootEx<CComMultiThreadModelNoCS>,

	public CComCoClass<CRAGEFolder, &CLSID_RPFXplorerStorageHandler>,

	//
	public IContextMenu,

	//
	public IShellExtInit,
	public IShellFolder2,
	public IPersistFolder2,

	//
	public IRPF
{
private:
	ATL::CString m_strFilePath;
	HANDLE m_hFile = NULL;
	RPFReader m_Reader;
	PIDLIST_ABSOLUTE m_pidl = NULL;
	RPFEntry* m_pCurrentDirectory = NULL;  // Current directory entry (NULL = root)
	bool m_bIsSubFolder = false;           // True if this is a subfolder view
	CRAGEFolder* m_pParentFolder = NULL;   // Parent folder (for subfolders)

public:
	CRAGEFolder()
	{
	}

	~CRAGEFolder()
	{
		if (m_hFile && m_hFile != INVALID_HANDLE_VALUE && !m_bIsSubFolder)
		{
			// Only close file handle if we're the root folder
			m_Reader.Close();
			CloseHandle(m_hFile);
			m_hFile = NULL;
		}

		if (m_pidl)
		{
			CoTaskMemFree(m_pidl);
			m_pidl = NULL;
		}

		if (m_pCurrentDirectory && m_bIsSubFolder)
		{
			delete m_pCurrentDirectory;
			m_pCurrentDirectory = NULL;
		}

		// Don't delete parent folder, it's managed by COM
		m_pParentFolder = NULL;
	}

	// Helper method to initialize as a subfolder
	HRESULT InitializeSubFolder(CRAGEFolder* pParent, const RPFEntry* pDirEntry)
	{
		if (!pParent || !pDirEntry)
			return E_INVALIDARG;

		if (pDirEntry->dwType != RPF_ENTRY_TYPE_DIRECTORY)
			return E_INVALIDARG;

		m_bIsSubFolder = true;
		m_pParentFolder = pParent;
		m_pParentFolder->AddRef(); // Keep parent alive

		// Copy the directory entry
		m_pCurrentDirectory = new RPFEntry();
		*m_pCurrentDirectory = *pDirEntry;

		// Share the file handle and reader from parent
		m_hFile = pParent->m_hFile;
		m_strFilePath = pParent->m_strFilePath;

		return S_OK;
	}

	//
	// IContextMenu
	//
	STDMETHOD(QueryContextMenu)			(_In_ HMENU hMenu, _In_ UINT uIndexMenu, _In_ UINT uIDCmdFirst, _In_ UINT uIDCmdLast, _In_ UINT uFlags) override;
	STDMETHOD(InvokeCommand)			(_In_ CMINVOKECOMMANDINFO* lpInfo) override;
	STDMETHOD(GetCommandString)			(_In_ UINT_PTR pIDCmd, _In_ UINT uType, _Reserved_ UINT* pReserved, _In_ CHAR* pszName, _In_ UINT cchMax) override;

	//
	// IShellExtInit
	//
	STDMETHOD(Initialize)				(_In_opt_ PCIDLIST_ABSOLUTE pidlFolder, _In_opt_ IDataObject* pdtobj, _In_opt_ HKEY hkeyProgID) override;

	//
	// IShellFolder
	//
	STDMETHOD(ParseDisplayName)			(_In_opt_ HWND hWnd, _In_opt_ LPBC pbc, _In_ LPOLESTR pszDisplayName, _Reserved_ LPDWORD pchEaten, _Out_ LPITEMIDLIST* ppidl, _Inout_ LPDWORD pdwAttributes) override;
	STDMETHOD(EnumObjects)				(_In_opt_ HWND hWnd, _In_ DWORD grfFlags, _Out_opt_ IEnumIDList** ppenumIDList) override;
	STDMETHOD(BindToObject)				(_In_ PCUIDLIST_RELATIVE pidl, _In_opt_ LPBC pbc, _In_ REFIID riid, _Out_opt_ void** ppv) override;
	STDMETHOD(BindToStorage)			(_In_ PCUIDLIST_RELATIVE pidl, _In_opt_ LPBC pbc, _In_ REFIID riid, _Out_opt_ void** ppv) override;
	STDMETHOD(CompareIDs)				(_In_ LPARAM lParam, _In_ PCUIDLIST_RELATIVE pidl1, _In_ PCUIDLIST_RELATIVE pidl2) override;
	STDMETHOD(CreateViewObject)			(_In_opt_ HWND hWndOwner, _In_ REFIID riid, _In_opt_ void** ppv) override;
	STDMETHOD(GetAttributesOf)			(_In_ UINT cidl, _In_ PCUITEMID_CHILD_ARRAY apidl, _Inout_ LPDWORD rgfInOut) override;
	STDMETHOD(GetUIObjectOf)			(_In_opt_ HWND hWndOwner, _In_ UINT cidl, _In_ PCUITEMID_CHILD_ARRAY apidl, _In_ REFIID riid, _Reserved_ LPUINT rgfReserved, _Out_opt_ void** ppv) override;
	STDMETHOD(GetDisplayNameOf)			(_In_opt_ PCUITEMID_CHILD pidl, _In_ DWORD uFlags, _Out_ LPSTRRET pName) override;
	STDMETHOD(SetNameOf)				(_In_opt_ HWND hWnd, _In_ PCUITEMID_CHILD pidl, _In_ LPCWSTR pszName, _In_ SHGDNF uFlags, _Out_opt_ PITEMID_CHILD* ppidlOut) override;

	//
	//  IShellFolder2
	//
	STDMETHOD(GetDefaultSearchGUID)		(_Out_ LPGUID pguid) override;
	STDMETHOD(EnumSearches)				(_Out_opt_ IEnumExtraSearch** ppenum) override;
	STDMETHOD(GetDefaultColumn)			(_In_ DWORD dwRes, _Out_ ULONG* pSort, _Out_ ULONG* pDisplay) override;
	STDMETHOD(GetDefaultColumnState)	(_In_ UINT iColumn, _Out_ SHCOLSTATEF* pcsFlags) override;
	STDMETHOD(GetDetailsEx)				(_In_opt_ PCUITEMID_CHILD pidl, _In_ const SHCOLUMNID* pscid, _Out_ VARIANT* pv) override;
	STDMETHOD(GetDetailsOf)				(_In_opt_ PCUITEMID_CHILD pidl, _In_ UINT iColumn, _In_ SHELLDETAILS* psd) override;
	STDMETHOD(MapColumnToSCID)			(_In_ UINT, _Out_ SHCOLUMNID*) override;

	//
	// IPersist
	//
	STDMETHOD(GetClassID)				(_Out_ CLSID* pClassID);

	//
	// IPersistFolder
	//
	STDMETHOD(Initialize)				(_In_ PCIDLIST_ABSOLUTE pidl);

	//
	// IPersistFolder2
	//
	STDMETHOD(GetCurFolder)				(_Out_ PIDLIST_ABSOLUTE* ppidl) override;

	//
	// IRPF
	//
	STDMETHOD(GetRPFReader)				(_Out_ RPFReader** ppReader) override;

	//
	// Helper methods
	//
	RPFEntry* GetCurrentDirectory();

public:
	DECLARE_NO_REGISTRY()
	DECLARE_NOT_AGGREGATABLE(CRAGEFolder)

	DECLARE_PROTECT_FINAL_CONSTRUCT()

	BEGIN_COM_MAP(CRAGEFolder)
		COM_INTERFACE_ENTRY_IID(IID_IShellFolder2,		IShellFolder2)
		COM_INTERFACE_ENTRY_IID(IID_IShellFolder,		IShellFolder)

		COM_INTERFACE_ENTRY_IID(IID_IPersistFolder2,	IPersistFolder2)
		COM_INTERFACE_ENTRY_IID(IID_IPersistFolder,		IPersistFolder)

		COM_INTERFACE_ENTRY_IID(IID_IPersist,			IPersist)

		COM_INTERFACE_ENTRY_IID(IID_IShellExtInit,		IShellExtInit)

		COM_INTERFACE_ENTRY_IID(IID_IContextMenu,		IContextMenu)
	END_COM_MAP()
};
